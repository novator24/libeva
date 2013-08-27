#include "../evastreamconcat.h"
#include "../evamemory.h"
#include "../evainit.h"
#include <string.h>

int main(int argc, char **argv)
{
  GskStream *out;
  char buf[10];
  guint nread;
  GError *error = NULL;

  eva_init_without_threads (&argc, &argv);
  out = eva_streams_concat_and_unref (eva_memory_source_static_string ("hi"),
                                      eva_memory_source_static_string ("mom"),
                                      NULL);
  nread = eva_stream_read (out, buf, 10, &error);
  g_assert (nread == 5);
  g_assert (eva_io_get_is_readable (out));
  g_assert (error == NULL);
  g_assert (memcmp (buf, "himom", 5) == 0);
  g_assert (eva_stream_read (out, buf, 10, &error) == 0);
  g_assert (!eva_io_get_is_readable (out));
  g_assert (error == NULL);
  g_object_unref (out);

  return 0;
}
