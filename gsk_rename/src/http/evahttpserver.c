#include <ctype.h>
#include <string.h>
#include "evahttpserver.h"
#include "../evamacros.h"

static GObjectClass *parent_class = NULL;

/* forward declarations for the "POST-stream" */
typedef struct _EvaHttpServerPostStream EvaHttpServerPostStream;
static EvaHttpServerPostStream *
eva_http_server_post_stream_new (EvaHttpServer         *server,
				 gboolean               is_chunked,
				 gint                   length);
static gboolean
eva_http_server_post_stream_process (EvaHttpServerPostStream *post_stream);
static void
eva_http_server_post_stream_detach (EvaHttpServerPostStream *post_stream,
				    gboolean                 is_server_dying);

/* amount of posted data to accumulate in the POST-data stream.
 */
#define MAX_POST_BUFFER		8192

typedef enum
{
  INIT,
  READING_REQUEST_FIRST_LINE,
  READING_REQUEST,
  READING_POST,
  DONE_READING
} ResponseParseState;

struct _EvaHttpServerResponse
{
  EvaHttpServer *server;
  GHashTable *request_parser_table;

  EvaHttpRequest *request;
  EvaHttpServerPostStream *post_data;

  ResponseParseState parse_state;

  /* all outgoing data goes here first temporarily */
  EvaBuffer      outgoing;
  guint content_received;

  /* members from eva_http_server_respond */
  EvaHttpResponse *response;
  EvaStream *content;

  /* have we written all the content out */
  guint is_done_writing : 1;

  /* have we gotten eof from 'content' */
  /* XXX: this flag should be removed: it is never meaningfully used.
          Instead content is set to NULL. */
  guint got_content_eof : 1;

  /* whether the user has queried this response yet */
  guint user_fetched : 1;

  /* whether this response failed for some reason */
  guint failed : 1;
  
  /* number of bytes of content written thus far */
  guint content_written;

  EvaHttpServerResponse *next;
};
EVA_DECLARE_POOL_ALLOCATORS(EvaHttpServerResponse, eva_http_server_response, 6)

struct _EvaHttpServerPostStreamClass 
{
  EvaStreamClass stream_class;
};

struct _EvaHttpServerPostStream 
{
  EvaStream      stream;
  EvaBuffer      buffer;
  EvaHttpServer *server;
  guint          blocking_server_write : 1;

  /* for Content-length: handling */
  guint          has_length : 1;

  /* for Transfer-Encoding: chunked */
  guint          is_chunked : 1;
  guint          is_in_chunk_header : 1;

  guint          ended : 1;

  /* for Content-Length/Chunking */
  guint          cur_size;
};

static inline gboolean
eva_http_server_response_is_done (EvaHttpServerResponse *response)
{
  if (response->failed)
    return TRUE;
  return response->parse_state == DONE_READING
      && response->is_done_writing
      && response->outgoing.size == 0
      && response->content == NULL;
}

static inline void
eva_http_server_response_destroy (EvaHttpServerResponse *response,
				  gboolean               is_server_dying)
{
  if (response->request)
    g_object_unref (response->request);
  if (response->post_data)
    {
      eva_http_server_post_stream_detach (response->post_data, is_server_dying);
      g_object_unref (response->post_data);
    }
  eva_buffer_destruct (&response->outgoing);

  if (response->response)
    g_object_unref (response->response);
  if (response->content)
    g_object_unref (response->content);
  eva_http_server_response_free (response);
}

static void eva_http_server_prune_done_responses (EvaHttpServer *server,
                                                  gboolean       may_read_shutdown);
static inline void
eva_http_server_response_fail (EvaHttpServerResponse *response,
			       const char            *explanation)
{
  /* NOTE: someday we're probably going to want a callback here. */ 
  GError *error = response->request
                ? EVA_HTTP_HEADER (response->request)->g_error
                : NULL;
  if (error == NULL)
    error = response->response
          ? EVA_HTTP_HEADER (response->response)->g_error
          : NULL;

#if 0
  if (error)
    g_debug ("eva_http_server_response_fail: %s: %s", explanation, error->message);
  else
    g_debug ("eva_http_server_response_fail: %s", explanation);
#endif

  response->failed = 1;
}

static gboolean
handle_content_is_readable (EvaStream *content_stream, gpointer data)
{
  EvaHttpServer *server = EVA_HTTP_SERVER (data);
  EvaHttpServerResponse *trapped_response = server->trapped_response;
  GError *error = NULL;
  gboolean was_empty;
  g_return_val_if_fail (trapped_response != NULL && trapped_response->content == content_stream, FALSE);
  was_empty = trapped_response->outgoing.size == 0;
  if (EVA_HTTP_HEADER (trapped_response->response)->transfer_encoding_type == EVA_HTTP_TRANSFER_ENCODING_CHUNKED)
    {
      /* TODO: temporary buffer pooling?? (so that large reads can be spared a copy...) */
      char buf[4096];
      char len_prefix[64];
      gsize n_read = eva_stream_read (content_stream, buf, sizeof (buf), &error);
      if (error != NULL)
	goto handle_error;
      if (n_read > 0)
        {
          g_snprintf (len_prefix, sizeof (len_prefix), "%x\r\n", (guint) n_read);
          eva_buffer_append_string (&trapped_response->outgoing, len_prefix);
          eva_buffer_append (&trapped_response->outgoing, buf, n_read);
          eva_buffer_append (&trapped_response->outgoing, "\r\n", 2);
          trapped_response->content_received += n_read;
        }
    }
  else
    {
      trapped_response->content_received += eva_stream_read_buffer (content_stream, &trapped_response->outgoing, &error);
    }
  if (error != NULL)
    goto handle_error;
  if (was_empty && trapped_response->outgoing.size > 0)
    eva_io_notify_ready_to_read (server);
  if (trapped_response->outgoing.size > 0)
    eva_io_mark_idle_notify_read (server);
  return TRUE;

handle_error:
  eva_io_set_gerror (EVA_IO (server), EVA_IO_ERROR_READ, error);
  server->trapped_response = NULL;
  return FALSE;
}

static gboolean
should_close_after_this_response (EvaHttpServerResponse *response)
{
  EvaHttpHeader *resp = EVA_HTTP_HEADER (response->response);
  return eva_http_header_get_connection (resp) == EVA_HTTP_CONNECTION_CLOSE;
}

static gboolean
handle_content_shutdown (EvaStream *content_stream, gpointer data)
{
  EvaHttpServer *server = EVA_HTTP_SERVER (data);
  EvaHttpServerResponse *trapped_response = server->trapped_response;
  gint content_length;
  g_return_val_if_fail (trapped_response != NULL && trapped_response->content == content_stream, FALSE);
  trapped_response->content = NULL;
  server->trapped_response = NULL;
  if (EVA_HTTP_HEADER (trapped_response->response)->transfer_encoding_type == EVA_HTTP_TRANSFER_ENCODING_CHUNKED)
    {
      gboolean was_empty = trapped_response->outgoing.size == 0;
      eva_buffer_append_string (&trapped_response->outgoing, "0\n");
      if (was_empty)
	eva_io_mark_idle_notify_read (server);
    }
  content_length = EVA_HTTP_HEADER (trapped_response->response)->content_length;
  if (content_length >= 0)
    {
      if (trapped_response->content_received != (guint)content_length)
        {
          eva_io_set_error (EVA_IO (server), EVA_IO_ERROR_READ,
                            EVA_ERROR_INVALID_STATE,
                            "expected %u bytes of data, got %u",
                            content_length,
                            trapped_response->content_received);
          g_object_unref (content_stream);
          return FALSE;
        }
    }
  if (trapped_response->outgoing.size == 0)
    {
      trapped_response->is_done_writing = 1;
      if (should_close_after_this_response (trapped_response))
        {
          eva_io_notify_read_shutdown (server);
          if (eva_io_get_is_writable (server))
            eva_io_write_shutdown (server, NULL);
        }
    }
  eva_http_server_prune_done_responses (server, TRUE);
  g_object_unref (content_stream);
  return FALSE;
}

static gboolean
handle_keepalive_idle_timeout (gpointer data)
{
  EvaHttpServer *server = EVA_HTTP_SERVER (data);
  server->keepalive_idle_timeout = NULL;
  eva_io_notify_shutdown (EVA_IO (server));
  return FALSE;
}

static inline void
add_keepalive_idle_timeout (EvaHttpServer *server)
{
  g_assert (server->keepalive_idle_timeout == NULL);
  g_assert (server->keepalive_idle_timeout_ms >= 0);
  server->keepalive_idle_timeout
    = eva_main_loop_add_timer (eva_main_loop_default (),
                               handle_keepalive_idle_timeout, server, NULL,
                               server->keepalive_idle_timeout_ms, -1);
}

static void
eva_http_server_prune_done_responses (EvaHttpServer *server,
                                      gboolean       may_read_shutdown)
{
  EvaHttpServerResponse **pthis = &server->first_response;
  EvaHttpServerResponse *last = NULL;
  EvaHttpServerResponse *at;
  while (*pthis != NULL)
    {
      EvaHttpServerResponse *at = *pthis;
      if (eva_http_server_response_is_done (at))
	{
          if (server->trapped_response == at)
	    {
	      if (at->content != NULL)
		eva_stream_untrap_readable (at->content);
	      server->trapped_response = NULL;
	    }
	  *pthis = at->next;
	  eva_http_server_response_destroy (at, FALSE);
	}
      else
	{
	  pthis = &at->next;
	  last = at;
	}
    }
  server->last_response = last;

  for (at = server->first_response; at != NULL; at = at->next)
    {
      if (!at->is_done_writing)
	break;
    }
  
  if ((at == NULL || at->is_done_writing)
   && (server->got_close || !eva_io_get_is_writable (server)))
    {
      /* The server is no longer readable.
	 This automatically clears the idle-notify flag. */
      if (may_read_shutdown)
        eva_io_notify_read_shutdown (server);
      else
        eva_io_set_idle_notify_read (server, TRUE);
      return;
    }

  eva_io_set_idle_notify_read (server, at != NULL && at->outgoing.size != 0);

  /* if we are blocking on the content-stream for data,
     trap its readability. */
  if (at != NULL && at->outgoing.size == 0 && at->content != NULL
   && server->read_poll && server->trapped_response != at)
    {
      /* Untrap the old stream (if applicable),
         and trap the correct one. */
      if (server->trapped_response != NULL
       && server->trapped_response->content != NULL)
        eva_stream_untrap_readable (at->content);
      server->trapped_response = at;
      eva_stream_trap_readable (at->content, handle_content_is_readable,
                                handle_content_shutdown,
                                g_object_ref (server), g_object_unref);
    }

  if (server->first_response == NULL
   && server->keepalive_idle_timeout_ms >= 0
   && server->keepalive_idle_timeout == NULL
   && server->incoming.size == 0)
    {
      add_keepalive_idle_timeout (server);
    }

}

/* --- i/o implementation --- */
static void
eva_http_server_set_poll_read (EvaIO *io, gboolean should_poll)
{
  EVA_HTTP_SERVER (io)->read_poll = should_poll;
}

static gboolean
eva_http_server_shutdown_read   (EvaIO      *io,
				 GError    **error)
{
  EvaHttpServer *server = EVA_HTTP_SERVER (io);
  EvaHttpServerResponse *at;
  guint n_to_shutdown = 0;
  EvaStream **to_shutdown;
  guint i;
  for (at = server->first_response; at != NULL; at = at->next)
    if (!at->is_done_writing)
      {
        eva_http_server_response_fail (at, "shutdown-read while data is still queued");
        if (at->content != NULL
         && eva_io_get_is_readable (at->content))
          n_to_shutdown++;
      }
  to_shutdown = g_newa (EvaStream *, n_to_shutdown);
  i = 0;
  for (at = server->first_response; at != NULL; at = at->next)
    if (!at->is_done_writing
     && at->content != NULL
     && eva_io_get_is_readable (at->content))
      to_shutdown[i++] = g_object_ref (at->content);
  g_assert (i == n_to_shutdown);
  for (i = 0; i < n_to_shutdown; i++)
    {
      eva_io_read_shutdown (to_shutdown[i], NULL);
      g_object_unref (to_shutdown[i]);
    }
  return TRUE;
}

static void
eva_http_server_set_poll_write (EvaIO *io, gboolean should_poll)
{
  EVA_HTTP_SERVER (io)->write_poll = should_poll;
}

static gboolean
eva_http_server_shutdown_write  (EvaIO      *io,
				 GError    **error)
{
  EvaHttpServer *server = EVA_HTTP_SERVER (io);
  EvaHttpServerResponse *at;
  for (at = server->first_response; at != NULL; at = at->next)
    {
      if (at->parse_state == READING_POST)
        {
          at->parse_state = DONE_READING;
          at->post_data->ended = 1;
          if (at->post_data->buffer.size == 0)
	    eva_io_notify_read_shutdown (at->post_data);
        }
      else if (at->parse_state != DONE_READING)
        eva_http_server_response_fail (at, "shutdown when not in done-reading state");
    }

  eva_http_server_prune_done_responses (server, TRUE);

  eva_io_read_shutdown (EVA_IO (server), NULL);

  eva_hook_notify_shutdown (EVA_HTTP_SERVER_HOOK (server));
  return TRUE;
}

static guint
eva_http_server_raw_read      (EvaStream     *stream,
			       gpointer       data,
			       guint          length,
			       GError       **error)
{
  EvaHttpServer *server = EVA_HTTP_SERVER (stream);
  EvaHttpServerResponse *at;
  guint rv;
  for (at = server->first_response; at != NULL; at = at->next)
    {
      if (!at->is_done_writing)
	break;
      if (at->response == NULL)
	{
	  eva_io_clear_idle_notify_read (server);
	  return 0;
	}
    }
  if (at == NULL)
    {
      eva_io_clear_idle_notify_read (server);
      if (server->got_close || !eva_io_get_is_writable (server))
        eva_io_notify_read_shutdown (server);
      return 0;
    }

  /* ok, 'at' is a response that may have data which can be read out */
  rv = 0;
  while (at != NULL && at->response != NULL)
    {
      if (at->outgoing.size > 0)
	{
	  guint amt  = MIN (length - rv, at->outgoing.size);
	  if (amt)
	    {
	      eva_buffer_read (&at->outgoing, (char *) data + rv, length - rv);
	      rv += amt;
	    }
	  if (rv == length)
	    break;
	}
      if (at->outgoing.size == 0 && at->content == NULL)
	{
	  /* ok, done with this response */
	  at->is_done_writing = TRUE;
	  if (eva_http_header_get_connection (EVA_HTTP_HEADER (at->response)) == EVA_HTTP_CONNECTION_CLOSE)
	    {
	      server->got_close = TRUE;
	      break;
	    }
	  at = at->next;
	}
      else
	{
          break;
	}
    }

  eva_http_server_prune_done_responses (server, rv == 0);

  return rv;
}

static EvaHttpServerResponse *
create_new_response (EvaHttpServer *server)
{
  EvaHttpServerResponse *response = eva_http_server_response_alloc ();
  response->server = server;
  response->request_parser_table = NULL;
  response->request = NULL;
  response->post_data = NULL;
  response->parse_state = INIT;
  eva_buffer_construct (&response->outgoing);
  response->content_received = 0;
  response->response = NULL;
  response->content = NULL;
  response->is_done_writing = 0;
  response->got_content_eof = 0;
  response->user_fetched = 0;
  response->content_written = 0;
  response->failed = 0;
  response->next = NULL;

  /* append this response to the queue */
  if (server->last_response)
    server->last_response->next = response;
  else
    server->first_response = response;
  server->last_response = response;

  return response;
}

static void
first_line_parser_callback  (EvaHttpServerResponse *response,
			     const char       *text)
{
  GError *error = NULL;
  g_assert (response->request == NULL);
  response->request = eva_http_request_new_blank ();

  switch (eva_http_request_parse_first_line (response->request, text, &error))
    {
    case EVA_HTTP_REQUEST_FIRST_LINE_ERROR:
      {
        EvaHttpRequest *request = response->request;
        response->request = NULL;
        eva_io_set_gerror (EVA_IO (response->server), EVA_IO_ERROR_WRITE, error);
        if (request != NULL)
          g_object_unref (request);
      }
      return;

    case EVA_HTTP_REQUEST_FIRST_LINE_SIMPLE:
      response->parse_state = DONE_READING;
      response->request_parser_table = eva_http_header_get_parser_table (TRUE);
      response->post_data = NULL;
      eva_hook_notify (EVA_HTTP_SERVER_HOOK (response->server));
      break;

    case EVA_HTTP_REQUEST_FIRST_LINE_FULL:
      response->parse_state = READING_REQUEST;
      response->request_parser_table = eva_http_header_get_parser_table (TRUE);
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
header_line_parser_callback (EvaHttpServerResponse *response,
			     const char            *line)
{
  EvaHttpHeaderLineParser *parser;
  char *lowercase;
  const char *colon;
  unsigned i;
  const char *val;
  if (line[0] == 0)
    {
      EvaHttpVerb verb = response->request->verb;
      if (verb == EVA_HTTP_VERB_PUT
       || verb == EVA_HTTP_VERB_POST)
	{
	  EvaHttpHeader *hdr = EVA_HTTP_HEADER (response->request);
	  gboolean chunked = hdr->transfer_encoding_type == EVA_HTTP_TRANSFER_ENCODING_CHUNKED;
	  gint content_length = hdr->content_length;
	  response->post_data = eva_http_server_post_stream_new (response->server,
							         chunked,
							         content_length);
	  response->parse_state = READING_POST;
	}
      else
	{
	  response->parse_state = DONE_READING;
	  response->post_data = NULL;
	}
      eva_hook_notify (EVA_HTTP_SERVER_HOOK (response->server));
      return;
    }

  colon = strchr (line, ':');
  if (colon == NULL)
    {
      g_warning ("no colon in header line");
      return;	/* XXX: error handling! */
    }

  /* lowercase the header */
  lowercase = g_alloca (colon - (char*)line + 1);
  for (i = 0; line[i] != ':'; i++)
    lowercase[i] = g_ascii_tolower (line[i]);
  lowercase[i] = '\0';
  
  parser = g_hash_table_lookup (response->request_parser_table, lowercase);
  if (parser == NULL)
    {
      /* XXX: error handling */
      gboolean is_nonstandard = (line[0] == 'x' || line[0] == 'X')
                              && line[1] == '-';
      if (!is_nonstandard)
        g_warning ("couldn't handle header line %s", line);
      return;
    }

  val = colon + 1;
  EVA_SKIP_WHITESPACE (val);
  if (! ((*parser->func) (EVA_HTTP_HEADER (response->request), val, parser->data)))
    {
      /* XXX: error handling */
      g_warning ("error parsing header line %s", line);
      return;
    }
}

#define MAX_STACK_ALLOC	4096

static guint
eva_http_server_raw_write     (EvaStream     *stream,
			       gconstpointer  data,
			       guint          length,
			       GError       **error)
{
  EvaHttpServer *server = EVA_HTTP_SERVER (stream);
  EvaHttpServerResponse *at;
  char stack_buf[MAX_STACK_ALLOC];

  if (length > 0 && server->keepalive_idle_timeout != NULL)
    {
      eva_source_remove (server->keepalive_idle_timeout);
      server->keepalive_idle_timeout = NULL;
    }

  /* TODO: need a zero-copy strategy */
  eva_buffer_append (&server->incoming, data, length);

  while (server->incoming.size > 0)
    {
      if (server->last_response != NULL
       && server->last_response->parse_state != DONE_READING)
        at = server->last_response;
      else
        at = create_new_response (server);

      switch (at->parse_state)
        {
        case INIT:
          at->parse_state = READING_REQUEST_FIRST_LINE;
          break;
        case READING_REQUEST_FIRST_LINE:
          {
            int nl = eva_buffer_index_of (&server->incoming, '\n');
            char *first_line;
            char *free_line = NULL;
            if (nl < 0)
              goto done;
            if (nl > MAX_STACK_ALLOC - 1)
              free_line = first_line = g_malloc (nl + 1);
            else
              first_line = stack_buf;
            eva_buffer_read (&server->incoming, first_line, nl + 1);
            first_line[nl] = '\0';
            g_strchomp (first_line);
            first_line_parser_callback (at, first_line);
            g_free (free_line);
          }
          break;

        case READING_REQUEST:
          {
            int nl = eva_buffer_index_of (&server->incoming, '\n');
            char *header_line;
            char *free_line = NULL;
            if (nl < 0)
              goto done;
            if (nl > MAX_STACK_ALLOC - 1)
              free_line = header_line = g_malloc (nl + 1);
            else
              header_line = stack_buf;
            eva_buffer_read (&server->incoming, header_line, nl + 1);
            header_line[nl] = '\0';
            g_strchomp (header_line);
            header_line_parser_callback (at, header_line);
            g_free (free_line);
          }
          break;
        case READING_POST:
          if (eva_http_server_post_stream_process (at->post_data))
            at->parse_state = DONE_READING;
          break;
        default:
          goto done;
        }
    }

done:
  return length;
}

static void
eva_http_server_finalize (GObject *object)
{
  EvaHttpServer *server = EVA_HTTP_SERVER (object);
  while (server->first_response)
    {
      EvaHttpServerResponse *response = server->first_response;
      server->first_response = response->next;
      eva_http_server_response_destroy (response, TRUE);
    }
  if (server->keepalive_idle_timeout != NULL)
    {
      eva_source_remove (server->keepalive_idle_timeout);
      server->keepalive_idle_timeout = NULL;
    }
  eva_buffer_destruct (&server->incoming);
  eva_hook_destruct (&server->has_request_hook);
  parent_class->finalize (object);
}

/* --- functions --- */
static void
eva_http_server_init (EvaHttpServer *http_server)
{
  EVA_HOOK_INIT (http_server, EvaHttpServer, has_request_hook, 0, 
		 set_poll_request, shutdown_request);
  EVA_HOOK_MARK_FLAG (&http_server->has_request_hook, IS_AVAILABLE);
  http_server->keepalive_idle_timeout_ms = -1;
  eva_io_mark_is_readable (http_server);
  eva_io_mark_is_writable (http_server);
  eva_io_set_idle_notify_write (http_server, TRUE);
}

static void
eva_http_server_class_init (EvaHttpServerClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  EvaStreamClass *stream_class = EVA_STREAM_CLASS (class);
  EvaIOClass *io_class = EVA_IO_CLASS (class);
  parent_class = g_type_class_peek_parent (class);
  stream_class->raw_read = eva_http_server_raw_read;
  stream_class->raw_write = eva_http_server_raw_write;
  object_class->finalize = eva_http_server_finalize;
  io_class->set_poll_read = eva_http_server_set_poll_read;
  io_class->shutdown_read = eva_http_server_shutdown_read;
  io_class->set_poll_write = eva_http_server_set_poll_write;
  io_class->shutdown_write = eva_http_server_shutdown_write;
  EVA_HOOK_CLASS_INIT (object_class, "request", EvaHttpServer, has_request_hook);
}

GType eva_http_server_get_type()
{
  static GType http_server_type = 0;
  if (!http_server_type)
    {
      static const GTypeInfo http_server_info =
      {
	sizeof(EvaHttpServerClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_http_server_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EvaHttpServer),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_http_server_init,
	NULL		/* value_table */
      };
      http_server_type = g_type_register_static (EVA_TYPE_STREAM,
                                                 "EvaHttpServer",
						 &http_server_info, 0);
    }
  return http_server_type;
}

/* === Implementation of Post/Put data streams === */
typedef struct _EvaHttpServerPostStreamClass EvaHttpServerPostStreamClass;
GType eva_http_server_post_stream_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_HTTP_SERVER_POST_STREAM			(eva_http_server_post_stream_get_type ())
#define EVA_HTTP_SERVER_POST_STREAM(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_HTTP_SERVER_POST_STREAM, EvaHttpServerPostStream))
#define EVA_HTTP_SERVER_POST_STREAM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_HTTP_SERVER_POST_STREAM, EvaHttpServerPostStreamClass))
#define EVA_HTTP_SERVER_POST_STREAM_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_HTTP_SERVER_POST_STREAM, EvaHttpServerPostStreamClass))
#define EVA_IS_HTTP_SERVER_POST_STREAM(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_HTTP_SERVER_POST_STREAM))
#define EVA_IS_HTTP_SERVER_POST_STREAM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_HTTP_SERVER_POST_STREAM))

static GObjectClass *post_stream_parent_class = NULL;

static inline void
check_unblock_server (EvaHttpServerPostStream *post_stream)
{
  if (post_stream->server
   && post_stream->blocking_server_write
   && post_stream->buffer.size < MAX_POST_BUFFER)
    {
      post_stream->blocking_server_write = FALSE;
      eva_io_unblock_write (post_stream->server);
    }
  if (post_stream->buffer.size == 0)
    {
      if (post_stream->ended)
	eva_io_notify_read_shutdown (post_stream);
      else
	eva_io_clear_idle_notify_read (post_stream);
    }
}

static guint
eva_http_server_post_stream_raw_read        (EvaStream     *stream,
					     gpointer       data,
					     guint          length,
					     GError       **error)
{
  EvaHttpServerPostStream *post_stream = EVA_HTTP_SERVER_POST_STREAM (stream);
  guint rv = MIN (length, post_stream->buffer.size);
  eva_buffer_read (&post_stream->buffer, data, rv);
  check_unblock_server (post_stream);
  return rv;
}

static guint
eva_http_server_post_stream_raw_read_buffer (EvaStream     *stream,
					     EvaBuffer     *buffer,
					     GError       **error)
{
  EvaHttpServerPostStream *post_stream = EVA_HTTP_SERVER_POST_STREAM (stream);
  guint rv = eva_buffer_drain (buffer, &post_stream->buffer);
  check_unblock_server (post_stream);
  return rv;
}

static void
eva_http_server_post_stream_finalize (GObject *object)
{
  EvaHttpServerPostStream *post_stream = EVA_HTTP_SERVER_POST_STREAM (object);
  eva_buffer_destruct (&post_stream->buffer);
  post_stream_parent_class->finalize (object);
}

static void
eva_http_server_post_stream_init (EvaHttpServerPostStream *server_post_stream)
{
  eva_stream_mark_is_readable (server_post_stream);
}

static void
eva_http_server_post_stream_class_init (EvaHttpServerPostStreamClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  EvaStreamClass *stream_class = EVA_STREAM_CLASS (class);
  post_stream_parent_class = g_type_class_peek_parent (class);
  stream_class->raw_read = eva_http_server_post_stream_raw_read;
  stream_class->raw_read_buffer = eva_http_server_post_stream_raw_read_buffer;
  object_class->finalize = eva_http_server_post_stream_finalize;
}

GType eva_http_server_post_stream_get_type()
{
  static GType http_server_post_stream_type = 0;
  if (!http_server_post_stream_type)
    {
      static const GTypeInfo http_server_post_stream_info =
      {
	sizeof(EvaHttpServerPostStreamClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_http_server_post_stream_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EvaHttpServerPostStream),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_http_server_post_stream_init,
	NULL		/* value_table */
      };
      http_server_post_stream_type = g_type_register_static (EVA_TYPE_STREAM,
                                                  "EvaHttpServerPostStream",
						  &http_server_post_stream_info, 0);
    }
  return http_server_post_stream_type;
}

static EvaHttpServerPostStream *
eva_http_server_post_stream_new (EvaHttpServer         *server,
				 gboolean               is_chunked,
				 gint                   length)
{
  EvaHttpServerPostStream *rv;
  rv = g_object_new (EVA_TYPE_HTTP_SERVER_POST_STREAM, NULL);
  rv->server = server;
  if (is_chunked)
    { 
      rv->is_chunked = 1;
      rv->is_in_chunk_header = 1;
    }
  else if (length >= 0)
    {
      rv->has_length = 1;
      rv->cur_size = length;
    }
  else
    {
      rv->has_length = 0;
    }
  return rv;
}


/* --- implement post data processing --- */

/* Process data for Transfer-Encoding: chunked.
   Returns whether the stream has ended. */
static inline gboolean
process_chunked (EvaHttpServerPostStream *post_stream)
{
  EvaBuffer *orig = &post_stream->server->incoming;
  for (;;)
    {
      if (post_stream->is_in_chunk_header)
	{
	  int c = eva_buffer_read_char (orig);
	  if (c == '\n')
	    {
	      post_stream->is_in_chunk_header = 0;
	      if (post_stream->cur_size == 0)
		return TRUE;
	    }
	  else if ('0' <= c && c <= '9')
	    {
	      post_stream->cur_size <<= 4;
	      post_stream->cur_size += (c - '0');
	      continue;
	    }
	  else if ('a' <= c && c <= 'f')
	    {
	      post_stream->cur_size <<= 4;
	      post_stream->cur_size += (c - 'a' + 10);
	      continue;
	    }
	  else if ('A' <= c && c <= 'F')
	    {
	      post_stream->cur_size <<= 4;
	      post_stream->cur_size += (c - 'A' + 10);
	      continue;
	    }
	  else if (c == -1)
	    break;
	  else
	    continue;
	}
      g_assert (!post_stream->is_in_chunk_header);
      {
	guint xfer = MIN (orig->size, post_stream->cur_size);
	eva_buffer_transfer (&post_stream->buffer, orig, xfer);
	post_stream->cur_size -= xfer;
	if (post_stream->cur_size == 0)
	  post_stream->is_in_chunk_header = 1;
	else
	  break;
      }
    }
  return FALSE;
}


/* Process data for Transfer-Encoding: identity.
   Returns whether the stream has ended. */
static inline gboolean
process_unencoded (EvaHttpServerPostStream *post_stream)
{
  EvaBuffer *orig = &post_stream->server->incoming;
  if (post_stream->has_length)
    {
      guint xfer = MIN (orig->size, post_stream->cur_size);
      eva_buffer_transfer (&post_stream->buffer, orig, xfer);
      post_stream->cur_size -= xfer;
      if (post_stream->cur_size == 0)
	return TRUE;
    }
  else
    eva_buffer_drain (&post_stream->buffer, orig);

  return FALSE;
}

/* Returns whether the post-data is over */
static gboolean
eva_http_server_post_stream_process (EvaHttpServerPostStream *post_stream)
{
  gboolean ended;
  if (post_stream->is_chunked)
    ended = process_chunked (post_stream);
  else
    ended = process_unencoded (post_stream);
  eva_io_set_idle_notify_read (EVA_IO (post_stream),
			       post_stream->buffer.size > 0);
  if (post_stream->buffer.size > MAX_POST_BUFFER
   && !post_stream->blocking_server_write
   && post_stream->server != NULL               /* always true, i think */
   && !ended)
    {
      post_stream->blocking_server_write = 1;
      eva_io_block_write (post_stream->server);
    }
  if (ended)
    {
      post_stream->ended = 1;
      if (post_stream->buffer.size == 0)
	eva_io_notify_read_shutdown (post_stream);
    }
  return ended;
}

static void
eva_http_server_post_stream_detach (EvaHttpServerPostStream *post_stream,
				    gboolean                 is_server_dying)
{
  if (!is_server_dying && post_stream->blocking_server_write)
    {
      post_stream->blocking_server_write = 0;
      eva_io_unblock_write (post_stream->server);
    }
  post_stream->server = NULL;
  post_stream->ended = 1;
  if (post_stream->buffer.size == 0)
    eva_io_notify_read_shutdown (post_stream);
}

/* --- public interface --- */
/**
 * eva_http_server_new:
 *
 * Allocate a new HTTP server protocol processor.
 * (Note that generally you will need to connect it
 * to an accepted socket)
 *
 * returns: the newly allocated server.
 */
EvaHttpServer *
eva_http_server_new (void)
{
  return g_object_new (EVA_TYPE_HTTP_SERVER, NULL);
}

/**
 * eva_http_server_get_request:
 * @server: the HTTP server to grab the request from.
 * @request_out: location to store a reference to the HTTP request.
 * @post_data_out: location to store a reference to the HTTP request's POST data,
 * or NULL will be stored if this is not a POST-type request.
 *
 * Grab a client request if available.  Use eva_http_server_trap()
 * to get notification when a request is available.
 *
 * The corresponding POST data stream must be retrieved at the 
 * same time.
 *
 * returns: whether a request was successfully dequeued.
 */
gboolean
eva_http_server_get_request (EvaHttpServer   *server,
			     EvaHttpRequest **request_out,
			     EvaStream      **post_data_out)
{
  EvaHttpServerResponse *sresponse;
  for (sresponse = server->first_response;
       sresponse != NULL;
       sresponse = sresponse->next)
    {
      if (!sresponse->user_fetched)
	{
	  EvaHttpRequest *request = sresponse->request;
	  EvaHttpServerPostStream *post_data = sresponse->post_data;
	  *request_out = g_object_ref (request);
	  if (post_data_out)
	    {
	      if (post_data)
		*post_data_out = g_object_ref (post_data);
	      else
		*post_data_out = NULL;
	    }
	  sresponse->user_fetched = 1;
	  return TRUE;
	}
    }
  return FALSE;
}

/**
 * eva_http_server_respond:
 * @server: the server to write the response to.
 * @request: the request obtained with eva_http_server_get_request().
 * @response: the response constructed to this request.
 * @content: content data if appropriate to this request.
 *
 * Give a response to a client's request.
 */
void
eva_http_server_respond     (EvaHttpServer   *server,
			     EvaHttpRequest  *request,
			     EvaHttpResponse *response,
			     EvaStream       *content)
{
  EvaHttpServerResponse *sresponse;
  g_return_if_fail (content == NULL || !eva_hook_is_trapped (EVA_IO_READ_HOOK (content)));
  g_return_if_fail (response != NULL);
  for (sresponse = server->first_response;
       sresponse != NULL;
       sresponse = sresponse->next)
    {
      if (sresponse->request == request)
	break;
    }
  g_return_if_fail (sresponse != NULL);
  if (sresponse->response != NULL)
    {
      g_warning ("got multiple responses to request for '%s'", request->path);
      return;
    }
  g_return_if_fail (sresponse->content == NULL);

  if (content != NULL && !EVA_HTTP_HEADER (response)->has_content_type)
    g_warning ("HTTP response has content but no Content-Type header");

  sresponse->response = g_object_ref (response);
  if (content)
    sresponse->content = g_object_ref (content);
  eva_http_header_to_buffer (EVA_HTTP_HEADER (response), &sresponse->outgoing);
  
  if (!eva_io_get_idle_notify_read (server))
    {
      for (sresponse = server->first_response;
	   sresponse != NULL;
	   sresponse = sresponse->next)
	{
	  if (!sresponse->is_done_writing)
	    {
	      if (!sresponse->request)
		return;
	      if (sresponse->outgoing.size == 0
		  && sresponse->content != NULL
		  && !sresponse->got_content_eof)
		return;
	      eva_io_mark_idle_notify_read (server);
	      return;
	    }
	}
    }
}

/* TODO: we should have an idle_time member
   so that we can start the timer from
   when the server went idle, as opposed to from
   the current time. */
void
eva_http_server_set_idle_timeout (EvaHttpServer *server,
                                  gint           millis)
{
  if (millis < 0)
    {
      if (server->keepalive_idle_timeout != NULL)
        {
          eva_source_remove (server->keepalive_idle_timeout);
          server->keepalive_idle_timeout = NULL;
        }
      server->keepalive_idle_timeout_ms = -1;
    }
  else
    {
      if (server->keepalive_idle_timeout != NULL)
        {
          eva_source_adjust_timer (server->keepalive_idle_timeout,
                                   millis, -1);
        }
      else if (server->first_response == NULL
            && server->incoming.size == 0)
        {
          add_keepalive_idle_timeout (server);
        }
    }
}

