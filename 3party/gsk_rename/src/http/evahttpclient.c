/*
 * State of an HTTP Client
 *
 * An HTTP client has a queue of requests,
 * maintained in the order in which they
 * are added with eva_http_client_request().
 *
 * The first request on the queue still is
 * waiting for input from the server.
 * A later request is being read from.
 *
 *   first_request -> .... -> last_request
 *       
 */    
#include <ctype.h>
#include <string.h>
#include "evahttpclient.h"
#include "../evamacros.h"
#include "../evalog.h"

/* TODO: contention on POST data:  right now, we buffer as much
   data as the user feels like giving us... need to block the hook.  */
#define BLOCK_POST_DATA 0       /* unfinished */

/* TODO: contention on content-stream.  right now, we "never_block_on_write"
   which means that if someone gives us gigabytes of data we
   will buffer it, instead of blocking it. */

/* TODO: if the content-stream is shutdown by the user,
   we should automatically drain all the data from it. */


/* TODO: instead of only flushing DONE requests in the raw_write
   call, we should be flushing them whereever we can.  */

/* number of bytes of data to buffer from the content stream */
#define EVA_HTTP_CLIENT_CONTENT_STREAM_MAX_BUFFER		8192

#define TEST_CLIENT_USER_FLAGS(client, flags)  \
  (((EVA_HTTP_CLIENT_HOOK (client)->user_flags) & (flags)) == (flags))
#define TEST_CLIENT_USER_FLAG(client, flag)    \
  TEST_CLIENT_USER_FLAGS(client, EVA_HTTP_CLIENT_##flag)

#if 0
#define DEBUG	g_message
#else
#define DEBUG(args...)
#endif

static GObjectClass *parent_class = NULL;
typedef struct _EvaHttpClientContentStream EvaHttpClientContentStream;
/* --- prototypes for the implementation of the content-stream --- */
static EvaHttpClientContentStream *
eva_http_client_content_stream_new (EvaHttpClient *client);
static guint
eva_http_client_content_stream_xfer     (EvaHttpClientContentStream *stream,
				         EvaBuffer                  *buffer,
				         gssize                      max_data);
static void
eva_http_client_content_stream_shutdown (EvaHttpClientContentStream *);


/* --- states that a particular pending request can be in --- */

typedef enum
{
  /* just initialized -- using minimum resources */
  INIT,

  /* transferred header into ascii format in `outgoing' buffer */
  WRITING_HEADER,

  /* the entire header has been read out -- now reading from post_data stream */
  POSTING,

  /* got shutdown from post stream but still have buffered data */
  POSTING_WRITING,

  /* no more data to be read from this request -
   * we're waiting for first line of the response (which is handled
   * by a different command table).
   */
  READING_RESPONSE_HEADER_FIRST_LINE,

  /* we're waiting for the appropriate data to be written into us.
   */
  READING_RESPONSE_HEADER,

  /* header was written in -- now waiting for content to be finished */
  READING_RESPONSE_CONTENT_NO_ENCODING,
  READING_RESPONSE_CONTENT_NO_ENCODING_NO_LENGTH,

  /* for chunked data (Transfer-Encoding: chunked) we use these two states:
   * the first means we're reading a newline-terminated number
   * and the second means we're reading that much data */
  READING_RESPONSE_CONTENT_CHUNK_HEADER,
  READING_RESPONSE_CONTENT_CHUNK_DATA,
  READING_RESPONSE_CONTENT_CHUNK_NEWLINE,	/* empty line after data */

  /* request waiting to be deleted.
   * This is the state when all data for this request/response
   * cycle has been written into us.  It is not necessarily
   * the case that the ContentStream has been finished, however.
   */
  DONE
} RequestState;

/* after the request has been written out we can move on to the
 * next request.   this macro checks to see if a request has reached
 * this point yet, according to its 'state' member.
 */
#define request_state_is_readonly(state)  ((state) >= READING_RESPONSE_HEADER_FIRST_LINE)


struct _EvaHttpClientRequest
{
  EvaHttpClient *client;
  EvaHttpRequest *request;
  EvaStream *post_data;
  EvaBuffer outgoing;

  EvaHttpClientResponse handle_response;
  gpointer handle_response_data;
  GDestroyNotify handle_response_destroy;

  EvaHttpResponse *response;
  EvaHttpClientContentStream *content_stream;
  GHashTable *response_command_table;

  RequestState state;
  guint64 remaining_data;  /* only during READING_RESPONSE_CONTENT_* */
  EvaHttpClientRequest *next;
};
static void eva_http_client_request_destroy (EvaHttpClientRequest *request);
#define GET_REMAINING_DATA_UINT(request) \
  ((guint) MIN ((guint64)(request)->remaining_data, (guint64)G_MAXUINT))
EVA_DECLARE_POOL_ALLOCATORS (EvaHttpClientRequest, eva_http_client_request, 8)

static inline gboolean
had_eof_terminated_post_data (EvaHttpClientRequest *request)
{
  EvaHttpTransferEncoding te = EVA_HTTP_HEADER (request->request)->transfer_encoding_type;
  return (request->request->verb == EVA_HTTP_VERB_POST
       || request->request->verb == EVA_HTTP_VERB_PUT)
      && te != EVA_HTTP_TRANSFER_ENCODING_CHUNKED
      && EVA_HTTP_HEADER (request->request)->content_length == -1;
}

static inline void
set_state_to_reading_response (EvaHttpClientRequest *request)
{
  /*EvaHttpHeader *reqheader = EVA_HTTP_HEADER (request->request);*/
  g_return_if_fail (request->state == POSTING_WRITING
		 || request->state == WRITING_HEADER
		 || request->state == POSTING);
  request->state = READING_RESPONSE_HEADER_FIRST_LINE;
  if (request->response)
    g_object_unref (request->response);
  request->response = eva_http_response_new_blank ();

  /* various conditions in which the HTTP client stream is no longer readable */
  if (had_eof_terminated_post_data (request))
   eva_io_notify_read_shutdown (request->client);
}

static void
flush_done_requests (EvaHttpClient *client)
{
  EvaHttpClientRequest *at;
  while (client->first_request != NULL
     &&  client->first_request->state == DONE)
    {
      EvaHttpClientRequest *request = client->first_request;
      g_assert (request->client == client);
      client->first_request = request->next;
      if (client->first_request == NULL)
        client->last_request = NULL;
      if (request == client->outgoing_request)
        client->outgoing_request = request->next;

      /* destroy request */
      request->next = NULL;		/* unnecesssary */
      eva_http_client_request_destroy (request);
    }
  for (at = client->first_request; at; at = at->next)
    g_assert (at->client == client);
}

/* --- POST/PUT data handling --- */
/* XXX: need Transfer-Encoding chunked support here! */
static gboolean
handle_post_data_readable (EvaStream            *stream,
			   EvaHttpClientRequest *request)
{
  GError *error = NULL;
  if (EVA_HTTP_HEADER (request->request)->transfer_encoding_type == EVA_HTTP_TRANSFER_ENCODING_CHUNKED)
    {
      EvaBuffer buffer = EVA_BUFFER_STATIC_INIT;
      if (eva_stream_read_buffer (stream, &buffer, &error) != 0)
        {
          eva_buffer_printf(&request->outgoing, "%x\n", (guint)buffer.size);
          eva_buffer_drain (&request->outgoing, &buffer);
        }
    }
  else
    eva_stream_read_buffer (stream, &request->outgoing, &error);

  if (error)
    {
      /* XXX: error handling! */
      g_warning ("error from post-data: %s", error->message);
      g_error_free (error);
      return FALSE;
    }

  if (request->client->outgoing_request == request
   && request->outgoing.size > 0)
    {
      eva_io_mark_idle_notify_read (request->client);
    }

#if BLOCK_POST_DATA
  if (request->outgoing.size > MAX_OUTGOING_BUFFERED_DATA)
    {
      if (!request->blocked_post_data)
        {
          request->blocked_post_data = TRUE;
          eva_stream_block_readable (stream);
        }
    }
#endif
  return TRUE;
}
static gboolean
handle_post_data_shutdown (EvaStream            *stream,
			   EvaHttpClientRequest *request)
{
  if (request->state == POSTING)
    {
      request->state = POSTING_WRITING;
      if (EVA_HTTP_HEADER (request->request)->transfer_encoding_type == EVA_HTTP_TRANSFER_ENCODING_CHUNKED)
        eva_buffer_append (&request->outgoing, "0\n", 2);

      if (request->outgoing.size == 0)
        set_state_to_reading_response (request);
    }

#if BLOCK_POST_DATA
  if (request->blocked_post_data)
    {
      request->blocked_post_data = FALSE;
      eva_stream_unblock_readable (stream);
    }
#endif

  return FALSE;
}
static void
handle_post_data_destroy (gpointer data)
{
  EvaHttpClientRequest *request = data;
  g_return_if_fail (EVA_IS_HTTP_CLIENT (request->client));

  /* this only happens on errors... */
  if (request->state == POSTING)
    request->state = POSTING_WRITING;

  /* free the post-data... */
  if (request->post_data)
    {
      EvaStream *old_post_data = request->post_data;
      request->post_data = NULL;
      g_object_unref (old_post_data);
    }
}


/* --- handle data from the response header --- */
static void
handle_firstline_header (EvaHttpClientRequest  *request,
			 const char            *line)
{
  DEBUG ("start of handle_firstline_header %s", line);
  if (!eva_http_response_process_first_line (request->response, line))
    {
      eva_debug ("Eva-Http-Parser",
                 "WARNING: couldn't handle initial header line %s",
                 line);
      return;
    }
  request->state = READING_RESPONSE_HEADER;
  request->response_command_table = eva_http_header_get_parser_table (FALSE);
  DEBUG ("initializing command table for request %p", request);
}

static void
handle_response_header   (EvaHttpClientRequest  *request,
			  const char            *line)
{
  EvaHttpHeaderLineParser *parser;
  EvaHttpHeader *response_header = EVA_HTTP_HEADER (request->response);
  char *lowercase;
  const char *colon;
  const char *val;
  unsigned i;
  EVA_SKIP_WHITESPACE (line);		/* unnecessary */
  if (*line == 0)
    {
      DEBUG ("got whitespace line");
      if (request->response->status_code == EVA_HTTP_STATUS_CONTINUE)
        {
          g_object_unref (request->response);
          request->response = eva_http_response_new_blank ();
          request->state = READING_RESPONSE_HEADER_FIRST_LINE;
          return;
        }

      request->content_stream = eva_http_client_content_stream_new (request->client);
      /* done parsing header */
      if (response_header->transfer_encoding_type == EVA_HTTP_TRANSFER_ENCODING_CHUNKED)
	request->state = READING_RESPONSE_CONTENT_CHUNK_HEADER;
      else if (response_header->content_length < 0)
	{
	  request->state = READING_RESPONSE_CONTENT_NO_ENCODING_NO_LENGTH;
	  request->remaining_data = -1;
	}
      else
	{
	  request->state = READING_RESPONSE_CONTENT_NO_ENCODING;
	  request->remaining_data = response_header->content_length;
	}

      if (request->handle_response)
	request->handle_response (request->request, request->response,
				  EVA_STREAM (request->content_stream),
				  request->handle_response_data);

      if (response_header->content_length == 0)
	{
	  request->state = DONE;
	  eva_http_client_content_stream_shutdown (request->content_stream);
	}

      return;
    }

  colon = strchr (line, ':');
  if (colon == NULL)
    {
      eva_debug ("Eva-Http-Parser", "no colon in header line %s", line);
      return;	/* XXX: error handling! */
    }

  /* lowercase the header */
  lowercase = g_alloca (colon - (char*)line + 1);
  for (i = 0; line[i] != ':'; i++)
    lowercase[i] = g_ascii_tolower (line[i]);
  lowercase[i] = '\0';

  val = colon + 1;
  EVA_SKIP_WHITESPACE (val);
  
  parser = g_hash_table_lookup (request->response_command_table, lowercase);
  if (parser == NULL)
    {
      /* XXX: error handling */
      gboolean is_nonstandard = (line[0] == 'x' || line[0] == 'X')
                              && line[1] == '-';
      if (!is_nonstandard)
        eva_debug ("Eva-Http-Parser", "couldn't handle header line %s", line);
      eva_http_header_add_misc (EVA_HTTP_HEADER (request->response),
                                lowercase, 
                                val);
      return;
    }

  if (! ((*parser->func) (EVA_HTTP_HEADER (request->response), val, parser->data)))
    {
      /* XXX: error handling */
      eva_debug ("Eva-Http-Parser", "error parsing header line %s", line);
      return;
    }
}


/* --- stream member functions --- */
static void
eva_http_client_set_poll_read   (EvaIO      *io,
				 gboolean    do_poll)
{
  EvaHttpClient *client = EVA_HTTP_CLIENT (io);
  g_assert (EVA_IS_HTTP_CLIENT (client));
  //client->has_read_poll = do_poll;
}

static void
eva_http_client_set_poll_write  (EvaIO      *io,
				 gboolean    do_poll)
{
  EvaHttpClient *client = EVA_HTTP_CLIENT (io);
  g_assert (EVA_IS_HTTP_CLIENT (client));
  //client->has_write_poll = do_poll;
}

static guint
eva_http_client_raw_read        (EvaStream     *stream,
				 gpointer       data,
				 guint          length,
				 GError       **error)
{
  EvaHttpClient *client = EVA_HTTP_CLIENT (stream);
  guint rv = 0;
  while (client->outgoing_request)
    {
      EvaHttpClientRequest *request = client->outgoing_request;
      if (request->state == INIT)
	{
	  /* write the header into the buffer */
	  eva_http_header_to_buffer (EVA_HTTP_HEADER (request->request),
			             &request->outgoing);
	  request->state = WRITING_HEADER;
	}
      if (request->state == WRITING_HEADER)
	{
	  rv += eva_buffer_read (&request->outgoing,
				 ((char *)data) + rv,
				 length - rv);
	  if (rv == length)
	    break;
	  if (request->outgoing.size == 0)
	    {
	      if (request->post_data == NULL)
		set_state_to_reading_response (request);
	      else
		{
		  request->state = POSTING;
		  eva_io_trap_readable (request->post_data,
					handle_post_data_readable,
					handle_post_data_shutdown,
					request,
					handle_post_data_destroy);
		}
	    }
	}
      if (request->state == POSTING)
	{
	  rv += eva_buffer_read (&request->outgoing,
				 ((char *)data) + rv,
				 length - rv);
	  if (rv == length)
	    break;
	}
      if (request->state == POSTING_WRITING)
	{
	  rv += eva_buffer_read (&request->outgoing,
				 ((char *)data) + rv,
				 length - rv);
	  if (request->outgoing.size == 0)
	    set_state_to_reading_response (request);
	  if (rv == length)
	    break;
	}
      if (request_state_is_readonly (request->state))
	{
	  client->outgoing_request = request->next;
	}
      else
	{
	  break;
	}
    }

  if (client->outgoing_request == NULL
   || (client->outgoing_request != NULL
    && request_state_is_readonly (client->outgoing_request->state)
    && (!EVA_HTTP_CLIENT_IS_FAST (client)
      || client->outgoing_request->next == NULL)))
    {
      DEBUG ("clearing idle-notify-read");
      eva_io_clear_idle_notify_read (client);

      /* This seems like a pretty good idea all the time,
         but some servers react negatively to halfshutdowns
         of this form, so we only want to do the read shutdown
         if we REALLY have to.  That means only if the
         remote server is being fed EOF-terminated POST data. */
      if (TEST_CLIENT_USER_FLAGS (client, EVA_HTTP_CLIENT_DEFERRED_SHUTDOWN
                                        | EVA_HTTP_CLIENT_REQUIRES_READ_SHUTDOWN))
        eva_io_notify_read_shutdown (client);
    }

  DEBUG ("eva_http_client_raw_read: returning %u bytes", rv);

  return rv;
}

#define MAX_STACK_ALLOC	4096

static inline guint
xdigit_to_number (char c)
{
  return ('0' <= c && c <= '9') ? (c - '0')
       : ('a' <= c && c <= 'f') ? (c - 'a' + 10)
       : ('A' <= c && c <= 'F') ? (c - 'A' + 10)
       : 0 /* XXX: should never happen */
       ;
}
static guint
eva_http_client_raw_write      (EvaStream     *stream,
				gconstpointer  data,
				guint          length,
				GError       **error)
{
  EvaHttpClient *client = EVA_HTTP_CLIENT (stream);

  char stack_buf[MAX_STACK_ALLOC];

  /* TODO: figure out a zero-copy strategy... */
  EvaBuffer *incoming_data = &client->incoming_data;
  eva_buffer_append (incoming_data, data, length);

  while (client->first_request != NULL
      && incoming_data->size > 0)
    {
      EvaHttpClientRequest *request = client->first_request;
      if (request->state == INIT
       || request->state == WRITING_HEADER)
	{
	  /* invalid request state. */
	  g_set_error (error, EVA_G_ERROR_DOMAIN,
		       EVA_ERROR_HTTP_PARSE,
		       _("got data from server before request had been issued "
			 "(request->state = %d)"),
		       request->state);
          flush_done_requests (client);
	  return length;
	}
      if (request->state == DONE)
        {
          flush_done_requests (client);
          continue;
        }
      if (request->state == POSTING
       || request->state == POSTING_WRITING)
        {
          //g_warning ("need to block writes til post done?");
          flush_done_requests (client);
          return length;
        }
      if (request->state == READING_RESPONSE_HEADER_FIRST_LINE)
	{
	  int nl = eva_buffer_index_of (incoming_data, '\n');
	  char *free_buffer = NULL;
	  char *line;
	  if (nl < 0)
	    break;
	  if (nl > MAX_STACK_ALLOC - 1)
	    line = free_buffer = g_malloc (nl + 1);
	  else
	    line = stack_buf;
	  eva_buffer_read (incoming_data, line, nl + 1);
	  line[nl] = 0;
	  g_strchomp (line);

	  handle_firstline_header (request, line);
	  g_free (free_buffer);

	  DEBUG ("eva_simple_parser_parse_callback succeeded");
	  if (request->state != READING_RESPONSE_HEADER)
	    {
	      g_set_error (error, EVA_G_ERROR_DOMAIN,
			   EVA_ERROR_HTTP_PARSE,
			   _("error parsing first line of response"));
              flush_done_requests (client);
	      return length;
	    }
	}

      while (request->state == READING_RESPONSE_HEADER)
	{
	  int nl = eva_buffer_index_of (incoming_data, '\n');
	  char *free_buffer = NULL;
	  char *line;
	  if (nl < 0)
	    goto done;
	  if (nl > MAX_STACK_ALLOC - 1)
	    line = free_buffer = g_malloc (nl + 1);
	  else
	    line = stack_buf;
	  eva_buffer_read (incoming_data, line, nl + 1);
	  line[nl] = 0;
	  g_strchomp (line);

	  DEBUG ("client: trying to handle a normal request line");
	  handle_response_header (request, line);
	  g_free (free_buffer);
	}
      if (request->state == READING_RESPONSE_CONTENT_NO_ENCODING_NO_LENGTH)
	{
	  EvaHttpClientContentStream *content_stream = request->content_stream;
	  eva_http_client_content_stream_xfer (content_stream, incoming_data, -1);
	}
      else if (request->state == READING_RESPONSE_CONTENT_NO_ENCODING)
	{
	  EvaHttpClientContentStream *content_stream = request->content_stream;
          guint amt = GET_REMAINING_DATA_UINT (request);
	  amt = eva_http_client_content_stream_xfer (content_stream, incoming_data, amt);
	  request->remaining_data -= amt;
	  if (request->remaining_data == 0)
	    {
	      request->state = DONE;
	      eva_http_client_content_stream_shutdown (content_stream);
	    }
	}
chunk_start:
      /* From RFC 2616, Section 3.6.1

         The chunk-size field is a string of hex digits indicating the size of
         the chunk. The chunked encoding is ended by any chunk whose size is
         zero, followed by the trailer, which is terminated by an empty line. */
      if (request->state == READING_RESPONSE_CONTENT_CHUNK_HEADER)
	{
	  int c;
	  while ((c = eva_buffer_read_char (incoming_data)) != -1)
	    {
	      if (isxdigit (c))
		{
		  request->remaining_data *= 16;
		  request->remaining_data += xdigit_to_number (c);
		}
	      else if (c == '\n')	/* TODO: read spec about which terminal char to use */
		{
		  if (request->remaining_data == 0)
		    {
		      request->state = DONE;
		      DEBUG ("client: got terminal chunk of length 0");
		      eva_http_client_content_stream_shutdown (request->content_stream);
		    }
		  else
		    {
		      request->state = READING_RESPONSE_CONTENT_CHUNK_DATA;
		      DEBUG ("chunk of data of length %u", request->remaining_data);
		    }
		  break;
		}
	    }
	  if (c == -1)
	    goto done;
	}
      if (request->state == READING_RESPONSE_CONTENT_CHUNK_DATA)
	{
          guint amt = GET_REMAINING_DATA_UINT (request);
	  EvaHttpClientContentStream *content_stream = request->content_stream;
	  amt = eva_http_client_content_stream_xfer (content_stream, incoming_data, amt);
	  request->remaining_data -= amt;
	  if (request->remaining_data == 0)
	    {
	      request->state = READING_RESPONSE_CONTENT_CHUNK_NEWLINE;
	    }
	}
      if (request->state == READING_RESPONSE_CONTENT_CHUNK_NEWLINE)
	{
	  int c;
	  while ((c = eva_buffer_read_char (incoming_data)) != -1)
	    if (c == '\n')
	      break;
	  if (c == -1)
	    goto done;
	  request->state = READING_RESPONSE_CONTENT_CHUNK_HEADER;
	  goto chunk_start;
	}

    }
done:
  flush_done_requests (client);
  if (client->first_request == NULL
   && TEST_CLIENT_USER_FLAG (client, DEFERRED_SHUTDOWN))
    {
      GError *error = NULL;
      eva_io_shutdown (EVA_IO (client), &error);
      /* XXX: error handling */
      if (error)
	g_warning ("error obeying deferred shutdown for http-client: %s", error->message);
    }
  return length;
}

#if 0		/* TODO: implement these for efficiency */
static guint
eva_http_client_raw_read_buffer (EvaStream     *stream,
				 EvaBuffer     *buffer,
				 GError       **error)
{
  EvaHttpClient *client = EVA_HTTP_CLIENT (stream);
  ...
}

static guint
eva_http_client_raw_write_buffer (EvaStream    *stream,
				  EvaBuffer     *buffer,
				  GError       **error)
{
  EvaHttpClient *client = EVA_HTTP_CLIENT (stream);
  ...
}
#endif

static gboolean
eva_http_client_shutdown_read  (EvaIO      *io,
			        GError    **error)
{
  EvaHttpClient *client = EVA_HTTP_CLIENT (io);
  EvaHttpClientRequest *request = client->first_request;
  while (request && request->state == DONE)
    request = request->next;
  if (request && request->state == POSTING)
    {
      /* TODO: error handling */
      eva_io_read_shutdown (request->post_data, NULL);
      request->state = READING_RESPONSE_HEADER_FIRST_LINE;
      request = request->next;
    }

  /* Warn about aborted requests. */
  {
    guint n_aborted = 0;
    while (request != NULL)
      {
	n_aborted++;
	request = request->next;
      }

      /* TODO: error handling */
    if (n_aborted > 0)
      eva_io_set_error (io, EVA_IO_ERROR_READ,
			EVA_ERROR_END_OF_FILE,
			_("due to transport shutdown, %u requests are being aborted"),
			n_aborted);
  }
  return TRUE;
}

static gboolean   
eva_http_client_shutdown_write (EvaIO      *io,
			        GError    **error)
{
  EvaHttpClient *client = EVA_HTTP_CLIENT (io);
  EvaHttpClientRequest *request = client->first_request;
  while (request && request->state == DONE)
    request = request->next;
  while (request != NULL
      && request->state >= READING_RESPONSE_HEADER_FIRST_LINE
      && request->state <= READING_RESPONSE_CONTENT_CHUNK_NEWLINE)
    {
      if (request->content_stream != NULL)
        eva_http_client_content_stream_shutdown (request->content_stream);
      request->state = DONE;
      request = request->next;
    }

  if (eva_io_get_is_readable (client))
    eva_io_read_shutdown (EVA_IO (client), NULL);

  flush_done_requests (client);

#if 0   /* XXX */
  while (request != NULL)
    {
      /* TODO: error handling */
      g_warning ("eva_http_client_shutdown_write causing request %p to abort", request);
      request = request->next;
    }
#endif
  return TRUE;
}

static void
eva_http_client_finalize (GObject *object)
{
  EvaHttpClient *client = EVA_HTTP_CLIENT (object);

  eva_hook_destruct (EVA_HTTP_CLIENT_HOOK (client));

  while (client->first_request != NULL)
    {
      EvaHttpClientRequest *request = client->first_request;
      client->first_request = request->next;
      if (client->first_request == NULL)
	client->last_request = NULL;

      request->next = NULL;		/* unnecesssary */
      eva_http_client_request_destroy (request);
    }
  eva_buffer_destruct (&client->incoming_data);

  (*parent_class->finalize) (object);
}

/* --- functions --- */
static void
eva_http_client_init (EvaHttpClient *http_client)
{
  EVA_HOOK_INIT (http_client, EvaHttpClient, requestable, 0, 
		 set_poll_requestable, shutdown_requestable);
  EVA_HOOK_SET_FLAG (EVA_HTTP_CLIENT_HOOK (http_client), IS_AVAILABLE);
  eva_io_mark_is_readable (http_client);
  eva_io_mark_is_writable (http_client);
  eva_io_mark_never_blocks_write (http_client);
}

static void
eva_http_client_class_init (EvaHttpClientClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  EvaIOClass *io_class = EVA_IO_CLASS (class);
  EvaStreamClass *stream_class = EVA_STREAM_CLASS (class);
  parent_class = g_type_class_peek_parent (class);
  io_class->set_poll_read = eva_http_client_set_poll_read;
  io_class->set_poll_write = eva_http_client_set_poll_write;
  io_class->shutdown_read = eva_http_client_shutdown_read;
  io_class->shutdown_write = eva_http_client_shutdown_write;
  stream_class->raw_read = eva_http_client_raw_read;
  stream_class->raw_write = eva_http_client_raw_write;
#if 0	/* TODO: implement these! */
  stream_class->raw_read_buffer = eva_http_client_raw_read_buffer;
  stream_class->raw_write_buffer = eva_http_client_raw_write_buffer;
#endif
  object_class->finalize = eva_http_client_finalize;
  EVA_HOOK_CLASS_INIT (object_class, "requestable", EvaHttpClient, requestable);
}

GType eva_http_client_get_type()
{
  static GType http_client_type = 0;
  if (!http_client_type)
    {
      static const GTypeInfo http_client_info =
      {
	sizeof(EvaHttpClientClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_http_client_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EvaHttpClient),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_http_client_init,
	NULL		/* value_table */
      };
      http_client_type = g_type_register_static (EVA_TYPE_STREAM,
                                                  "EvaHttpClient",
						  &http_client_info, 0);
    }
  return http_client_type;
}

/* --- public methods --- */

/**
 * eva_http_client_new:
 *
 * Create a new HTTP client protocol stream.
 * Note that usually you want to hook this up to
 * a transport layer, typically from eva_stream_new_connecting().
 *
 * returns: the new client protocol stream.
 */
EvaHttpClient *
eva_http_client_new (void)
{
  return g_object_new (EVA_TYPE_HTTP_CLIENT, NULL);
}

/**
 * eva_http_client_notify_fast:
 * @client: the http client to affect.
 * @is_fast: whether to try to get requests without waiting for responses.
 *
 * Set whether the client should allow and want multiple outgoing requests 
 * before a single response has been received.
 */
void
eva_http_client_notify_fast (EvaHttpClient     *client,
			     gboolean           is_fast)
{
  EVA_HTTP_CLIENT_HOOK (client)->user_flags |= EVA_HTTP_CLIENT_FAST_NOTIFY;
}

/**
 * eva_http_client_request:
 * @client: the http client to transmit the outgoing request.
 * @request: a request which should be sent from the client.
 * @post_data: for PUT and POST requests, a stream of data to output.
 * @handle_response: function to call once an HTTP response header is received.
 * @hook_data: data to pass to @handle_response.
 * @hook_destroy: method to call when the response is called or the request
 *    is aborted.
 *
 * Queue or send a HTTP client request.
 * The @handle_response function will be called once the response
 * header is available, and the content data will be available as
 * a #EvaStream.
 */
void
eva_http_client_request  (EvaHttpClient         *client,
			  EvaHttpRequest        *http_request,
			  EvaStream             *post_data,
			  EvaHttpClientResponse  handle_response,
			  gpointer               hook_data,
			  GDestroyNotify         hook_destroy)
{
  EvaHttpClientRequest *request;

  /* you must not issue another request after one which
     uses EOF to mark the end-of-request cycle:
     that request can never by processed. */
  g_return_if_fail (!TEST_CLIENT_USER_FLAG (client, REQUIRES_READ_SHUTDOWN));

  /* you must not issue another request after calling
     eva_http_client_shutdown_when_done() */
  g_return_if_fail (!TEST_CLIENT_USER_FLAG (client, DEFERRED_SHUTDOWN));

  request = eva_http_client_request_alloc ();
  request->client = client;
  request->request = g_object_ref (http_request);
  request->post_data = post_data ? g_object_ref (post_data) : NULL;
  eva_buffer_construct (&request->outgoing);
  request->handle_response = handle_response;
  request->handle_response_data = hook_data;
  request->handle_response_destroy = hook_destroy;
  request->response = NULL;
  request->content_stream = NULL;
  request->response_command_table = NULL;
  request->state = INIT;
  request->remaining_data = 0;
  request->next = NULL;

  if (client->last_request)
    client->last_request->next = request;
  else
    client->first_request = request;
  client->last_request = request;
  if (client->outgoing_request == NULL)
    client->outgoing_request = request;

  DEBUG ("eva_http_client_request: path=%s", http_request->path);

  if (post_data != NULL
   && EVA_HTTP_HEADER (http_request)->content_length < 0
   && EVA_HTTP_HEADER (http_request)->transfer_encoding_type == EVA_HTTP_TRANSFER_ENCODING_NONE)
    EVA_HTTP_CLIENT_HOOK (client)->user_flags |= EVA_HTTP_CLIENT_REQUIRES_READ_SHUTDOWN;


  eva_io_mark_idle_notify_read (client);
}

/**
 * eva_http_client_shutdown_when_done:
 * @client: the http client shut down.
 *
 * Set the client to shutdown after the current requests are done.
 */
void
eva_http_client_shutdown_when_done (EvaHttpClient *client)
{
  EVA_HTTP_CLIENT_HOOK (client)->user_flags |= EVA_HTTP_CLIENT_DEFERRED_SHUTDOWN;
  if (client->first_request == NULL)
    {
      GError *error = NULL;
      eva_io_shutdown (EVA_IO (client), &error);

      /* TODO: error handling */
      if (error)
	{
	  g_warning ("error shutting down http-client: %s", error->message);
	  g_error_free (error);
	}
    }
}

/**
 * eva_http_client_propagate_content_read_shutdown:
 * @client: the http client to affect.
 *
 * Propagate shutdowns of the content-stream to the http-client.
 */
void
eva_http_client_propagate_content_read_shutdown (EvaHttpClient *client)
{
  EVA_HTTP_CLIENT_HOOK (client)->user_flags |= EVA_HTTP_CLIENT_PROPAGATE_CONTENT_READ_SHUTDOWN;
}

static void
eva_http_client_request_destroy (EvaHttpClientRequest *request)
{
  if (request->request)
    g_object_unref (request->request);
  if (request->post_data)
    g_object_unref (request->post_data);
  eva_buffer_destruct (&request->outgoing);
  if (request->handle_response_destroy)
    request->handle_response_destroy (request->handle_response_data);
  if (request->response)
    g_object_unref (request->response);
  if (request->content_stream)
    {
      eva_http_client_content_stream_shutdown (request->content_stream);
      g_object_unref (request->content_stream);
    }
  eva_http_client_request_free (request);
}

/* === Implementation of the content-stream === */
typedef struct _EvaHttpClientContentStreamClass EvaHttpClientContentStreamClass;

GType eva_http_client_content_stream_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_HTTP_CLIENT_CONTENT_STREAM			(eva_http_client_content_stream_get_type ())
#define EVA_HTTP_CLIENT_CONTENT_STREAM(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_HTTP_CLIENT_CONTENT_STREAM, EvaHttpClientContentStream))
#define EVA_HTTP_CLIENT_CONTENT_STREAM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_HTTP_CLIENT_CONTENT_STREAM, EvaHttpClientContentStreamClass))
#define EVA_HTTP_CLIENT_CONTENT_STREAM_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_HTTP_CLIENT_CONTENT_STREAM, EvaHttpClientContentStreamClass))
#define EVA_IS_HTTP_CLIENT_CONTENT_STREAM(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_HTTP_CLIENT_CONTENT_STREAM))
#define EVA_IS_HTTP_CLIENT_CONTENT_STREAM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_HTTP_CLIENT_CONTENT_STREAM))

struct _EvaHttpClientContentStreamClass 
{
  EvaStreamClass stream_class;
};
struct _EvaHttpClientContentStream 
{
  EvaStream      stream;
  EvaBuffer      buffer;
  EvaHttpClient *http_client;
  guint          has_shutdown : 1;
  guint          has_client_write_block : 1;
};
/* --- prototypes --- */
static GObjectClass *content_stream_parent_class = NULL;

/* propagating write-blockage up the chain */
static inline void
eva_http_client_content_stream_mark_client_write_block (EvaHttpClientContentStream *stream)
{
  if (!stream->has_client_write_block)
    {
      stream->has_client_write_block = 1;
      if (stream->http_client)
	eva_io_block_write (stream->http_client);
    }
}

static inline void
eva_http_client_content_stream_clear_client_write_block (EvaHttpClientContentStream *stream)
{
  if (stream->has_client_write_block)
    {
      stream->has_client_write_block = 0;
      if (stream->http_client)
	eva_io_unblock_write (stream->http_client);
    }
}

/* io implementation */
static gboolean
eva_http_client_content_stream_shutdown_read   (EvaIO      *io,
						GError    **error)
{
  EvaHttpClientContentStream *content_stream = EVA_HTTP_CLIENT_CONTENT_STREAM  (io);
  /* XXX: why do we need to do this? */
  if (content_stream->http_client != NULL
   && (content_stream->http_client->last_request == NULL
    || content_stream->http_client->last_request->content_stream == content_stream)
   &&    (TEST_CLIENT_USER_FLAG (content_stream->http_client, PROPAGATE_CONTENT_READ_SHUTDOWN)
       || TEST_CLIENT_USER_FLAGS (content_stream->http_client,
                              EVA_HTTP_CLIENT_DEFERRED_SHUTDOWN
                            | EVA_HTTP_CLIENT_REQUIRES_READ_SHUTDOWN)))
    {
      /* this should be true, since eva_http_client_request()
         does not allow additional requests after DEFERRED_SHUTDOWN
         or REQUIRES_READ_SHUTDOWN. */
      g_assert (content_stream->http_client->last_request == NULL
             || content_stream->http_client->last_request->next == NULL);
      eva_io_notify_shutdown (EVA_IO (content_stream->http_client));
    }
  eva_http_client_content_stream_clear_client_write_block (content_stream);
  return TRUE;
}

/* stream implementation */
static inline void
eva_http_client_content_stream_read_fixups (EvaHttpClientContentStream *content_stream)
{
  if (content_stream->buffer.size == 0)
    eva_io_clear_idle_notify_read (content_stream);
  else
    eva_io_mark_idle_notify_read (content_stream);
  if (content_stream->buffer.size < EVA_HTTP_CLIENT_CONTENT_STREAM_MAX_BUFFER
   && content_stream->http_client != NULL)
    {
      eva_http_client_content_stream_clear_client_write_block (content_stream);
    }

  /* hmm, might this be too reentrant? */
  if (content_stream->buffer.size == 0
   && content_stream->has_shutdown)
    {
      eva_io_notify_read_shutdown (content_stream);
    }
}

static guint
eva_http_client_content_stream_raw_read        (EvaStream     *stream,
			 	                gpointer       data,
			 	                guint          length,
			 	                GError       **error)
{
  EvaHttpClientContentStream *content_stream = EVA_HTTP_CLIENT_CONTENT_STREAM (stream);
  guint rv = eva_buffer_read (&content_stream->buffer, data, length);
  DEBUG ("eva_http_client_content_stream_raw_read: max-length=%u; got=%u [buffer-size=%u] [has shutdown=%u]", length, rv, content_stream->buffer.size, content_stream->has_shutdown);
  eva_http_client_content_stream_read_fixups (content_stream);
  return rv;
}

static guint
eva_http_client_content_stream_raw_read_buffer (EvaStream     *stream,
			                        EvaBuffer     *buffer,
			                        GError       **error)
{
  EvaHttpClientContentStream *content_stream = EVA_HTTP_CLIENT_CONTENT_STREAM (stream);
  guint rv = eva_buffer_drain (buffer, &content_stream->buffer);
  DEBUG ("eva_http_client_content_stream_raw_read_buffer: got=%u", rv);
  eva_http_client_content_stream_read_fixups (content_stream);
  return rv;
}

static void
eva_http_client_content_stream_finalize (GObject *object)
{
  EvaHttpClientContentStream *content_stream = EVA_HTTP_CLIENT_CONTENT_STREAM (object);
  g_return_if_fail (content_stream->http_client == NULL);
  eva_buffer_destruct (&content_stream->buffer);
  content_stream_parent_class->finalize (object);
}

static void
eva_http_client_content_stream_init (EvaHttpClientContentStream *http_client_content_stream)
{
  eva_io_mark_is_readable (http_client_content_stream);
}

static void
eva_http_client_content_stream_class_init (EvaStreamClass *class)
{
  EvaIOClass *io_class = EVA_IO_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  content_stream_parent_class = g_type_class_peek_parent (class);
  io_class->shutdown_read = eva_http_client_content_stream_shutdown_read;
  class->raw_read = eva_http_client_content_stream_raw_read;
  class->raw_read_buffer = eva_http_client_content_stream_raw_read_buffer;
  object_class->finalize = eva_http_client_content_stream_finalize;
}

GType eva_http_client_content_stream_get_type()
{
  static GType http_client_content_stream_type = 0;
  if (!http_client_content_stream_type)
    {
      static const GTypeInfo http_client_content_stream_info =
      {
	sizeof(EvaHttpClientContentStreamClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_http_client_content_stream_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EvaHttpClientContentStream),
	4,		/* n_preallocs */
	(GInstanceInitFunc) eva_http_client_content_stream_init,
	NULL		/* value_table */
      };
      http_client_content_stream_type = g_type_register_static (EVA_TYPE_STREAM,
                                                  "EvaHttpClientContentStream",
						  &http_client_content_stream_info, 0);
    }
  return http_client_content_stream_type;
}

/* forward declared methods on EvaHttpClientContentStream */
static EvaHttpClientContentStream *
eva_http_client_content_stream_new (EvaHttpClient *client)
{
  EvaHttpClientContentStream *rv;
  rv = g_object_new (EVA_TYPE_HTTP_CLIENT_CONTENT_STREAM, NULL);
  rv->http_client = client;
  return EVA_HTTP_CLIENT_CONTENT_STREAM (rv);
}

static guint
eva_http_client_content_stream_xfer     (EvaHttpClientContentStream *stream,
				         EvaBuffer                  *buffer,
				         gssize                      max_data)
{
  guint amount;
  g_return_val_if_fail (!stream->has_shutdown, 0);
  if (max_data < 0)
    amount = eva_buffer_drain (&stream->buffer, buffer);
  else
    amount = eva_buffer_transfer (&stream->buffer, buffer, max_data);
  DEBUG ("eva_http_client_content_stream_xfer: content-stream size is %u; buffer now size %u [maxdata=%d]", stream->buffer.size, buffer->size, max_data);
  if (stream->buffer.size > 0)
    {
      eva_io_mark_idle_notify_read (stream);
    }
  if (stream->buffer.size > EVA_HTTP_CLIENT_CONTENT_STREAM_MAX_BUFFER
   && eva_io_get_is_readable (stream))
    {
      g_return_val_if_fail (stream->http_client != NULL, 0);
      eva_http_client_content_stream_mark_client_write_block (stream);
    }
  else if (!eva_io_get_is_readable (stream))
    {
      /* save memory if the data is just to be discarded */
      eva_buffer_destruct (&stream->buffer);
    }

  return amount;
}

static void
eva_http_client_content_stream_shutdown (EvaHttpClientContentStream *stream)
{
  DEBUG("eva_http_client_content_stream_shutdown: has_shutdown=%u; buffer.size=%u; idle-notify-read=%u", stream->has_shutdown,stream->buffer.size, eva_io_get_idle_notify_read(stream));
  if (stream->has_shutdown)
    return;
  stream->has_shutdown = 1;
  eva_http_client_content_stream_clear_client_write_block (stream);
  stream->http_client = NULL;
  if (stream->buffer.size == 0)
    eva_io_notify_read_shutdown (stream);
}
