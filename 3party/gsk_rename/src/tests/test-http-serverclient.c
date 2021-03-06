#include "../http/evahttpserver.h"
#include "../http/evahttpclient.h"
#include "../evamainloop.h"
#include "../evamemory.h"
#include "../evainit.h"
#include <string.h>

static EvaHttpClient *client = NULL;
static EvaHttpServer *server = NULL;

static EvaHttpRequest *server_request = NULL;
static gboolean        had_post_content;
static EvaBuffer       post_content_buffer = EVA_BUFFER_STATIC_INIT;
static gboolean        server_got_request = FALSE;

static EvaHttpResponse *client_response = NULL;
static EvaBuffer       client_content = EVA_BUFFER_STATIC_INIT;
static gboolean        has_response_content;
static gboolean        client_got_response = FALSE;

#if 0
#define DEBUG g_message
#else
#define DEBUG(args...)
#endif

static gboolean
handle_post_content_readable (EvaStream *stream, gpointer data)
{
  g_assert (data == NULL);
  eva_stream_read_buffer (stream, &post_content_buffer, NULL);
  return TRUE;
}
static gboolean
handle_post_content_shutdown (EvaStream *stream, gpointer data)
{
  g_assert (data == NULL);
  server_got_request = TRUE;
  return FALSE;
}

static gboolean
handle_server_trap (EvaHttpServer *server,
		    gpointer       data)
{
  EvaStream *server_post_content = NULL;
  DEBUG ("handle_server_trap");
  g_assert (data == NULL);
  g_assert (!server_got_request);
  g_assert (server_request == NULL);
  g_assert (eva_http_server_get_request (server, &server_request, &server_post_content));
  had_post_content = (server_post_content != NULL);
  eva_buffer_destruct (&post_content_buffer);

  if (!had_post_content)
    server_got_request = TRUE;
  else
    {
      eva_stream_trap_readable (server_post_content, handle_post_content_readable, handle_post_content_shutdown, NULL, NULL);
      g_object_unref (server_post_content);
    }

  DEBUG ("handle_post_content_shutdown: server_got_request = %d", server_got_request);

  return TRUE;
}
static gboolean
handle_server_shutdown (EvaHttpServer *server,
		        gpointer       data)
{
  g_assert (data == NULL);
  g_warning ("server shutdown");
  return FALSE;
}

static gboolean
client_handle_content_body (EvaStream *stream, gpointer data)
{
  DEBUG("client_handle_content_body");
  eva_stream_read_buffer (stream, &client_content, NULL);
  return TRUE;
}

static gboolean
client_handle_content_shutdown (EvaStream *stream, gpointer data)
{
  DEBUG("client_handle_content_shutdown");
  client_got_response = TRUE;
  return FALSE;
}

static void
client_handle_server_response (EvaHttpRequest  *request,
			       EvaHttpResponse *response,
			       EvaStream       *input,
			       gpointer         hook_data)
{
  g_assert (hook_data == NULL);
  g_assert (!client_got_response);
  client_response = g_object_ref (response);
  has_response_content = (input != NULL);
  if (has_response_content)
    {
      eva_stream_trap_readable (g_object_ref (input),
				client_handle_content_body,
				client_handle_content_shutdown,
				input,
				g_object_unref);
    }
  else
    {
      client_got_response = TRUE;
    }
}

static void
reset_transaction ()
{
  if (server_request)
    {
      g_object_unref (server_request);
      server_request = NULL;
    }
  if (client_response)
    {
      g_object_unref (client_response);
      client_response = NULL;
    }
  eva_buffer_destruct (&client_content);
  eva_buffer_destruct (&post_content_buffer);
  client_got_response = FALSE;
  server_got_request = FALSE;
}

static void
new_client_server ()
{
  client = eva_http_client_new ();
  server = eva_http_server_new ();
  eva_stream_attach_pair (EVA_STREAM (client), EVA_STREAM (server), NULL);
  eva_http_server_trap (server, handle_server_trap, handle_server_shutdown, NULL, NULL);
}

static void
clear_client_server ()
{
  eva_http_server_untrap (server);
  eva_io_shutdown (EVA_IO (client), NULL);
  eva_io_shutdown (EVA_IO (server), NULL);
  g_object_unref (client);
  g_object_unref (server);
  client = NULL;
  server = NULL;
}

int main(int argc, char **argv)
{
  EvaHttpRequest *client_request;
  EvaHttpResponse *response;
  EvaStream *content;
  EvaMainLoop *loop;
  int pass, i;
  eva_init_without_threads (&argc,&argv);
  loop = eva_main_loop_default ();

  new_client_server ();

  /* Do a trivial GET and successful response. */
  for (pass = 0; pass < 2; pass++)
    {
      for (i = 0; i < 5; i++)
	{
	  char buf[256];
	  g_printerr ("GET and response using %s...[iter %d] ",
		   pass == 0 ? "Transfer-Encoding: chunked" : "Content-Length",
		   i);

	  client_request = eva_http_request_new (EVA_HTTP_VERB_GET, "/");
	  eva_http_client_request (client, client_request, NULL, client_handle_server_response, NULL, NULL);
	  g_object_unref (client_request);
	  while (!server_got_request)
	    eva_main_loop_run (loop, -1, NULL);
	  DEBUG ("done server_got_request loop");
	  g_assert (!had_post_content);
	  g_assert (post_content_buffer.size == 0);
	  g_assert (server_request->verb == EVA_HTTP_VERB_GET);
	  g_assert (strcmp (server_request->path, "/") == 0);
	  g_snprintf (buf, sizeof (buf), "hi mom %d", i);
	  response = eva_http_response_from_request (server_request,
						     EVA_HTTP_STATUS_OK,
						     pass == 0 ? -1 : 8);
	  eva_http_response_set_content_type (response, "text");
	  eva_http_response_set_content_subtype (response, "plain");
	  if (pass == 0)
	    g_assert (EVA_HTTP_HEADER (response)->transfer_encoding_type == EVA_HTTP_TRANSFER_ENCODING_CHUNKED);
	  else
	    g_assert (EVA_HTTP_HEADER (response)->transfer_encoding_type == EVA_HTTP_TRANSFER_ENCODING_NONE);
	  content = eva_memory_source_new_printf ("%s", buf);
	  eva_http_server_respond (server, server_request, response, content);
	  g_object_unref (content);
	  g_object_unref (response);
	  content = NULL;
	  response = NULL;
	  while (!client_got_response)
	    eva_main_loop_run (loop, -1, NULL);
	  g_assert (client_content.size == 8);
	  {
	    char buf[8]; eva_buffer_read (&client_content, buf, 8);
	    g_assert (memcmp (buf, "hi mom ", 7) == 0);
	    g_assert (buf[7] == '0' + i);
	  }
	  g_assert (client_response->status_code == EVA_HTTP_STATUS_OK);
	  reset_transaction ();
	  g_printerr ("Ok\n");
	}
    }

  clear_client_server ();

  /* Test one-shot connections.  This can happen if:
   * pass=0: an HTTP 1.0 client is detected.
   * pass=1: the server deliberately sets the connection-type to close.
   */
  for (pass = 0; pass < 2; pass++)
    {
      for (i = 0; i < 5; i++)
	{
	  char buf[256];
	  new_client_server ();

	  g_printerr ("GET and response using %s...[iter %d] ",
		   pass == 0 ? "HTTP 1.0" : "server close",
		   i);

	  client_request = eva_http_request_new (EVA_HTTP_VERB_GET, "/");
	  if (pass == 0)
	    eva_http_header_set_version (EVA_HTTP_HEADER (client_request), 1, 0);
	  eva_http_client_request (client, client_request, NULL, client_handle_server_response, NULL, NULL);
	  g_object_unref (client_request);
	  while (!server_got_request)
	    eva_main_loop_run (loop, -1, NULL);
	  DEBUG ("done server_got_request loop");
	  g_assert (!had_post_content);
	  g_assert (post_content_buffer.size == 0);
	  g_assert (server_request->verb == EVA_HTTP_VERB_GET);
	  g_assert (strcmp (server_request->path, "/") == 0);
	  g_snprintf (buf, sizeof (buf), "hi mom %d", i);
	  response = eva_http_response_from_request (server_request,
						     EVA_HTTP_STATUS_OK,
						     pass == 0 ? -1 : 8);
	  if (pass == 1)
	    {
	      EVA_HTTP_HEADER (response)->transfer_encoding_type = EVA_HTTP_TRANSFER_ENCODING_NONE;
	      EVA_HTTP_HEADER (response)->connection_type = EVA_HTTP_CONNECTION_CLOSE;
	    }
	  g_assert (EVA_HTTP_HEADER (response)->transfer_encoding_type == EVA_HTTP_TRANSFER_ENCODING_NONE);
	  content = eva_memory_source_new_printf ("%s", buf);
	  eva_http_response_set_content_type (response, "text");
	  eva_http_response_set_content_subtype (response, "plain");
	  eva_http_server_respond (server, server_request, response, content);
	  g_object_unref (content);
	  g_object_unref (response);
	  content = NULL;
	  response = NULL;
	  while (!client_got_response)
	    eva_main_loop_run (loop, -1, NULL);
	  g_assert (client_content.size == 8);
	  {
	    char buf[8]; eva_buffer_read (&client_content, buf, 8);
	    g_assert (memcmp (buf, "hi mom ", 7) == 0);
	    g_assert (buf[7] == '0' + i);
	  }
	  g_assert (client_response->status_code == EVA_HTTP_STATUS_OK);
	  g_assert (eva_http_client_is_requestable (client));
	  reset_transaction ();
	  g_printerr ("Ok\n");
	  clear_client_server ();
	}
    }

  /* Test HEAD request */
  {
    static EvaHttpStatus codes[2] = { 200, 404 };
    new_client_server ();

    g_printerr ("HEAD request and response... ");

    for (i = 0; i < 2; i++)
      {
	client_request = eva_http_request_new (EVA_HTTP_VERB_HEAD, "/random-url");
	eva_http_client_request (client, client_request, NULL, client_handle_server_response, NULL, NULL);
	g_object_unref (client_request);
	while (!server_got_request)
	  eva_main_loop_run (loop, -1, NULL);
	g_assert (!had_post_content);
	g_assert (post_content_buffer.size == 0);
	g_assert (server_request->verb == EVA_HTTP_VERB_HEAD);
	g_assert (strcmp (server_request->path, "/random-url") == 0);
	response = eva_http_response_from_request (server_request, codes[i], 0);
	eva_http_server_respond (server, server_request, response, NULL);
	while (!client_got_response)
	  eva_main_loop_run (loop, -1, NULL);
	g_assert (client_response->status_code == codes[i]);
	g_assert (eva_http_client_is_requestable (client));
	g_object_unref (response);
	response = NULL;
	reset_transaction ();
	g_printerr (".");
      }
    g_printerr (" Ok.\n");
    clear_client_server ();
  }

  return 0;
}
