#include "../http/evahttpcontent.h"
#include "../http/evahttpclient.h"
#include "../http/evahttpserver.h"
#include "../evamemory.h"
#include "../evainit.h"
#include "../evamainloop.h"
#include <string.h>

typedef struct _ResponseData ResponseData;
struct _ResponseData
{
  gboolean drained;
  EvaHttpResponse *response;
  EvaBuffer content;
};

static void
handle_buffer_done (EvaBuffer *buffer, gpointer data)
{
  ResponseData *rd = data;
  eva_buffer_drain (&rd->content, buffer);
  rd->drained = TRUE;
}

static void
handle_response (EvaHttpRequest  *request,
                 EvaHttpResponse *response,
                 EvaStream       *input,
                 gpointer         hook_data)
{
  ResponseData *rd = hook_data;
  EvaStream *sink = eva_memory_buffer_sink_new (handle_buffer_done, rd, NULL);
  rd->response = g_object_ref (response);
  eva_stream_attach (input, sink, NULL);
}

static void
test_get (EvaHttpContent   *content,
          const char       *path,
          const char       *user_agent,
          EvaHttpResponse **response_out,
          char            **text_out)
{
  EvaHttpClient *client = eva_http_client_new ();
  EvaHttpServer *server = eva_http_server_new ();
  EvaHttpRequest *request = eva_http_request_new (EVA_HTTP_VERB_GET, path);
  ResponseData rd = {FALSE, NULL, EVA_BUFFER_STATIC_INIT};
  eva_http_content_manage_server (content, server);
  if (user_agent != NULL)
    g_object_set (request, "user-agent", user_agent, NULL);
  eva_http_client_request (client, request, NULL,
                           handle_response, &rd, NULL);
  eva_stream_attach_pair (EVA_STREAM (client), EVA_STREAM (server), NULL);

  while (!rd.drained)
    eva_main_loop_run (eva_main_loop_default (), 0, NULL);
  *response_out = rd.response;
  *text_out = g_malloc (rd.content.size + 1);
  (*text_out)[rd.content.size] = 0;
  eva_buffer_read (&rd.content, *text_out, rd.content.size);
  g_assert (rd.response);
}

static void
add_static_text (EvaHttpContent *content,
                 const EvaHttpContentId *id,
                 const char *static_string)
{
  eva_http_content_add_data (content, id, static_string, strlen (static_string), NULL, NULL);
}

int main(int argc, char **argv)
{
  EvaHttpContent *content;
  EvaHttpContentId id = EVA_HTTP_CONTENT_ID_INIT;
  EvaHttpResponse *response;
  char *data;
  const char *type, *subtype;
  eva_init_without_threads (&argc, &argv);
  content = eva_http_content_new ();

  id.path = "/hi/my/name/is/q.html";
  add_static_text (content, &id, "this is q.html");
  id.path = NULL;

  id.path_prefix = "/hi/my/name/prefix";
  add_static_text (content, &id, "anything under ...my/name/prefix");

  id.path_suffix = ".a";
  add_static_text (content, &id, "anything under ...my/name/prefix ... .a");
  id.path_suffix = ".b";
  add_static_text (content, &id, "anything under ...my/name/prefix ... .b");
  id.path_prefix = NULL;
  id.path_suffix = NULL;

  id.path = "/data/foo.b";
  add_static_text (content, &id, "some b2 data");
  id.path = NULL;

  eva_http_content_set_mime_type (content, NULL, ".html", "text", "html");
  eva_http_content_set_mime_type (content, NULL, ".a", "x-application", "a");
  eva_http_content_set_mime_type (content, "/hi", ".b", "x-application", "b");
  eva_http_content_set_mime_type (content, "/data", ".b", "x-application", "b2");

  g_assert (eva_http_content_get_mime_type (content, "whatever.html", &type, &subtype));
  g_assert (strcmp (type, "text") == 0 && strcmp (subtype, "html") == 0);
  g_assert (eva_http_content_get_mime_type (content, "whatever.a", &type, &subtype));
  g_assert (strcmp (type, "x-application") == 0 && strcmp (subtype, "a") == 0);
  g_assert (eva_http_content_get_mime_type (content, "/hi/whatever.b", &type, &subtype));
  g_assert (strcmp (type, "x-application") == 0 && strcmp (subtype, "b") == 0);
  g_assert (eva_http_content_get_mime_type (content, "/data/whatever.b", &type, &subtype));
  g_assert (strcmp (type, "x-application") == 0 && strcmp (subtype, "b2") == 0);
  g_assert (!eva_http_content_get_mime_type (content, "foo", &type, &subtype));

  eva_http_content_set_default_mime_type (content, "x-application", "x-unknown");
  g_assert (eva_http_content_get_mime_type (content, "foo", &type, &subtype));
  g_assert (strcmp (type, "x-application") == 0 && strcmp (subtype, "x-unknown") == 0);

  test_get (content, "/hi/my/name/is/q.html", NULL, &response, &data);
  g_assert (strcmp (data, "this is q.html") == 0);
  g_assert (strcmp (EVA_HTTP_HEADER (response)->content_type, "text") == 0);
  g_assert (strcmp (EVA_HTTP_HEADER (response)->content_subtype, "html") == 0);
  g_object_unref (response);
  g_free (data);

  test_get (content, "/hi/my/name/prefix/whatever/whatever", NULL, &response, &data);
  g_assert (strcmp (data, "anything under ...my/name/prefix") == 0);
  g_object_unref (response);
  g_free (data);

  test_get (content, "/hi/my/name/prefix/whatever/whatever.a", NULL, &response, &data);
  g_assert (strcmp (data, "anything under ...my/name/prefix ... .a") == 0);
  g_assert (strcmp (EVA_HTTP_HEADER (response)->content_type, "x-application") == 0);
  g_assert (strcmp (EVA_HTTP_HEADER (response)->content_subtype, "a") == 0);
  g_object_unref (response);
  g_free (data);

  test_get (content, "/hi/my/name/prefix/whatever/whatever2.b", NULL, &response, &data);
  g_assert (strcmp (data, "anything under ...my/name/prefix ... .b") == 0);
  g_assert (strcmp (EVA_HTTP_HEADER (response)->content_type, "x-application") == 0);
  g_assert (strcmp (EVA_HTTP_HEADER (response)->content_subtype, "b") == 0);
  g_object_unref (response);
  g_free (data);

  test_get (content, "/data/foo.b", NULL, &response, &data);
  g_assert (strcmp (data, "some b2 data") == 0);
  g_assert (strcmp (EVA_HTTP_HEADER (response)->content_type, "x-application") == 0);
  g_assert (strcmp (EVA_HTTP_HEADER (response)->content_subtype, "b2") == 0);
  g_object_unref (response);
  g_free (data);

  return 0;
}
