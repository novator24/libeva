#include "../evabufferstream.h"
#include "../evainit.h"

int main(int argc, char **argv)
{
  guint i;
  guint usecs;
  GTimeVal start, end;

  eva_init (&argc, &argv, NULL);

  g_get_current_time (&start);
  for (i = 0; i < 1000 * 1000; i++)
    {
      g_object_unref (g_object_new (EVA_TYPE_BUFFER_STREAM, NULL));
    }
  g_get_current_time (&end);

  usecs = (1000 * 1000 * (end.tv_sec - start.tv_sec))
        +                (end.tv_usec - start.tv_usec);
  g_message ("took %u microseconds", usecs);
  return 0;
}
