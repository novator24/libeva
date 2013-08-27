#include <string.h>
#include <stdlib.h>
#include "../http/evahttpcontent.h"
#include "../url/evaurltransferhttp.h"
#include "../evamemory.h"
#include "../evainit.h"

/* /a => /b */
/* /b => /c.html contents: "hi from /c.html" */
/* /d => /e */
/* /e => /d */
/* /f => /f */

/* SO: retrieve /a: should get /c.html
                /b: should get /c.html
                /c.html: should get /c.html
       retrieve /d, /e, /f to get circular redirect errors.
 */

static EvaHttpContentResult
redirect_handler (EvaHttpContent *content,
                  EvaHttpContentHandler *handler,
                  EvaHttpServer  *server,
                  EvaHttpRequest *request,
                  EvaStream      *post_data,
                  gpointer        data)
{
  EvaHttpResponse *res = eva_http_response_new_redirect ((char*)data);
  EvaStream *str = eva_memory_source_static_string ("redirect");
  eva_http_header_set_content_type (EVA_HTTP_HEADER (res), "text");
  eva_http_header_set_content_subtype (EVA_HTTP_HEADER (res), "plain");
  eva_http_server_respond (server, request, res, str);
  g_object_unref (res);
  g_object_unref (str);
  return EVA_HTTP_CONTENT_OK;
}
static void
add_redirect (EvaHttpContent *content,
              const char     *source,
              const char     *dest)
{
  EvaHttpContentId id = EVA_HTTP_CONTENT_ID_INIT;
  EvaHttpContentHandler *handler;
  id.path = source;
  handler = eva_http_content_handler_new (redirect_handler, g_strdup (dest), g_free);
  eva_http_content_add_handler (content, &id, handler, EVA_HTTP_CONTENT_REPLACE);
  eva_http_content_handler_unref (handler);
}

typedef struct _TransferData TransferData;
struct _TransferData
{
  gboolean transfer_done;
  gboolean got_stream;
  gboolean stream_done;
  gboolean transfer_destroyed;
  EvaBuffer incoming;
  GError *error;
};

static gboolean
handle_content_readable (EvaStream *stream, gpointer data)
{
  TransferData *td = data;
  g_message ("handle_content_readable");
  eva_stream_read_buffer (stream, &td->incoming, &td->error);
  return TRUE;
}

static gboolean
handle_content_read_shutdown (EvaStream *stream, gpointer data)
{
  g_object_unref (stream);
  return FALSE;
}

static void
set_stream_done (gpointer data)
{
  TransferData *td = data;
  g_message ("set_stream_done");
  td->stream_done = TRUE;
}

static void
handle_transfer (EvaUrlTransfer *transfer,
                 gpointer        data)
{
  TransferData *td = data;
  g_assert (!td->transfer_done);
  td->transfer_done = TRUE;
  if (transfer->error)
    {
      td->error = g_error_copy (transfer->error);
    }
  else if (transfer->result == EVA_URL_TRANSFER_REDIRECT)
    {
      td->error = g_error_new (EVA_G_ERROR_DOMAIN, EVA_ERROR_HTTP_PARSE,
                               "redirect encountered");
    }
  else
    {
      g_assert (transfer->result == EVA_URL_TRANSFER_SUCCESS);
      g_assert (transfer->content != NULL);
      td->got_stream = TRUE;
      eva_io_trap_readable (g_object_ref (transfer->content),
                            handle_content_readable,
                            handle_content_read_shutdown,
                            td, set_stream_done);
    }
}

static void
transfer_data_destroyed (gpointer data)
{
  TransferData *td = data;
  g_assert (!td->transfer_destroyed);
  td->transfer_destroyed = TRUE;
}

static gboolean
transfer (guint                port,
          const char          *path,
          gboolean             do_redirects,
          EvaUrlTransferHttp **transfer_out,
          guint               *len_out,
          char               **content_out,
          GError             **error)
{
  EvaUrl *url = eva_url_new_from_parts (EVA_URL_SCHEME_HTTP,
                                        "localhost", port,
                                        NULL, NULL, path, NULL, NULL);
  EvaUrlTransfer *transfer = eva_url_transfer_new (url);
  TransferData td;
  memset (&td, 0, sizeof (td));
  eva_buffer_construct (&td.incoming);
  eva_url_transfer_set_follow_redirects (transfer, do_redirects);
  eva_url_transfer_set_handler (transfer, handle_transfer, &td, transfer_data_destroyed);
  if (!eva_url_transfer_start (transfer, error))
    {
      g_object_unref (transfer);
      return FALSE;
    }
  while (!td.stream_done
      && td.error == NULL)
    {
      eva_main_loop_run (eva_main_loop_default (), -1, NULL);
    }

  if (td.error)
    {
      eva_buffer_destruct (&td.incoming);
      g_propagate_error (error, td.error);
      g_object_unref (transfer);
      return FALSE;
    }
  *transfer_out = EVA_URL_TRANSFER_HTTP (transfer);
  *len_out = td.incoming.size;
  *content_out = g_malloc (td.incoming.size + 1);
  (*content_out)[td.incoming.size] = 0;
  eva_buffer_read (&td.incoming, *content_out, td.incoming.size);

  eva_buffer_destruct (&td.incoming);
  return TRUE;
}

int main (int argc, char **argv)
{
  EvaHttpContent *content;
  guint port = 0;
  gint i;
  guint do_redirects;
  eva_init_without_threads (&argc, &argv);
  for (i = 1; i < argc; i++)
    {
      if (g_str_has_prefix (argv[i], "--port="))
        {
          port = atoi (strchr (argv[i], '=') + 1);
        }
      else
        g_error ("unknown argument %s", argv[i]);
    }
  if (port == 0)
    g_error ("missing --port= argument");
  content = eva_http_content_new ();
  add_redirect (content, "/a", "/b");
  add_redirect (content, "/b", "/c.html");
  add_redirect (content, "/d", "/e");
  add_redirect (content, "/e", "/d");
  add_redirect (content, "/f", "/f");
  eva_http_content_add_data_by_path (content, "/c.html", "hi mom", 6, NULL, NULL);
  eva_http_content_set_mime_type (content, NULL, ".html", "text", "html");

  guint len;
  char *str;
  GError *error = NULL;
  EvaUrlTransferHttp *htransfer;
  EvaSocketAddress *addr;

  addr = eva_socket_address_ipv4_localhost (port);
  if (!eva_http_content_listen (content, addr, &error))
    g_error ("eva_http_content_listen failed: %s", error->message);

  for (do_redirects = 0; do_redirects < 2; do_redirects++)
    {
      if (!transfer (port, "/c.html", do_redirects, &htransfer, &len, &str, &error))
        g_error  ("error transfering /c.html: %s", error->message);
      g_object_unref (htransfer);
      g_assert (len == 6);
      g_assert (strcmp (str, "hi mom") == 0);
      g_free (str);
    }

  if (!transfer (port, "/b", TRUE, &htransfer, &len, &str, &error))
    g_error  ("error transfering /b: %s", error->message);
  g_object_unref (htransfer);
  g_assert (len == 6);
  g_assert (strcmp (str, "hi mom") == 0);
  g_free (str);

  if (!transfer (port, "/a", TRUE, &htransfer, &len, &str, &error))
    g_error  ("error transfering /a: %s", error->message);
  g_object_unref (htransfer);
  g_assert (len == 6);
  g_assert (strcmp (str, "hi mom") == 0);
  g_free (str);

  if (transfer (port, "/d", TRUE, &htransfer, &len, &str, &error))
    g_error ("transfer from circular redirect succeeded???");
  /* TODO: assert error messsage or something?? */
  g_message ("error message from transfering /d: %s", error->message);
  g_clear_error (&error);

  if (transfer (port, "/e", TRUE, &htransfer, &len, &str, &error))
    g_error ("transfer from circular redirect succeeded???");
  /* TODO: assert error messsage or something?? */
  g_message ("error message from transfering /e: %s", error->message);
  g_clear_error (&error);

  if (transfer (port, "/f", TRUE, &htransfer, &len, &str, &error))
    g_error ("transfer from circular redirect succeeded???");
  /* TODO: assert error messsage or something?? */
  g_message ("error message from transfering /f: %s", error->message);
  g_clear_error (&error);

  return 0;
}
