#include <string.h>
#include "../eva.h"
#include "../zlib/evazlibinflator.h"
#include "../zlib/evazlibdeflator.h"


static gboolean use_gzip = FALSE;
static gboolean compress = FALSE;
static int flush_millis = -1;
static gint compress_level = -1;


static GOptionEntry entries[] =
{
  { "gzip", 'g', 0, G_OPTION_ARG_NONE, &use_gzip, "use gzip format (as opposed to deflate format)", NULL },
  { "compress", 'c', 0, G_OPTION_ARG_NONE, &compress, "compress (instead of decompressing)", NULL },
  { "flush-millis", 'f', 0, G_OPTION_ARG_INT, &flush_millis, "flush timeout", "MS" },
  { "compress-level", 0, 0, G_OPTION_ARG_INT, &compress_level, "compression level", NULL },
  { NULL, 0, 0, 0, NULL, NULL, NULL },
};

int
main (int argc, char **argv)
{
  EvaStream *in, *zlib, *out;
  GOptionContext *context;
  GError *error = NULL;
  eva_init_without_threads (&argc, &argv);

  context = g_option_context_new ("test-zlib-stream");
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    g_error ("error parsing options: %s", error->message);

  in = eva_stream_fd_new_auto (0);
  if (compress)
    zlib = eva_zlib_deflator_new2 (compress_level,
                                   flush_millis,
                                   use_gzip);
  else
    zlib = eva_zlib_inflator_new2 (use_gzip);
  out = eva_stream_fd_new_auto (1);
  eva_stream_attach (in, zlib, NULL);
  eva_stream_attach (zlib, out, NULL);
  g_object_weak_ref (G_OBJECT (zlib), (GWeakNotify) eva_main_quit, NULL);
  g_object_unref (zlib);
  return eva_main_run ();
}
