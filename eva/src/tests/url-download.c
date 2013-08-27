#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>			/* for STDOUT_FILENO */
#include "../url/evaurl.h"
#include "../evadebug.h"
#include "../evainit.h"
#include "../evamain.h"
#include "../evastreamfd.h"


static void
started_download  (GskStream        *stream,
		   gpointer          user_data)
{
  GskStream *stdout_stream = eva_stream_fd_new_auto (STDOUT_FILENO);
  g_printerr ("Starting download...\n");
  eva_stream_attach (stream, stdout_stream, NULL);
  g_object_weak_ref (G_OBJECT (stdout_stream), (GWeakNotify) eva_main_quit, NULL);
  g_object_unref (stdout_stream);
}

static void
download_failed   (GError           *error,
		   gpointer          user_data)
{
  g_error ("download_failed: %s", (error ? error->message : "?error unknown?"));
}

int main (int argc, char **argv)
{
  char *url_string;
  GskUrl *url;
  GError *error = NULL;

  eva_init_without_threads (&argc, &argv);

  //eva_debug_add_flags (EVA_DEBUG_ALL);
  
  url_string = argv[1];

  if (argc != 2)
    g_error ("%s requires exactly 1 argument, url", argv[0]);

  url = eva_url_new (argv[1], &error);
  if (url == NULL)
    g_error ("error parsing url: %s", error->message);
  g_message("scheme:\t%s",url->scheme_name);
  g_message("host:\t%s",url->host);
  g_message("path:\t%s",url->path);
  g_message("query:\t%s",url->query);
  g_message("fragment:\t%s",url->fragment);
  g_message("port:%d",url->port);

  eva_url_download (url, started_download, download_failed, url);
  g_object_unref (url);

  return eva_main_run ();
}
