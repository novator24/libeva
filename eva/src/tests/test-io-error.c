#include "../evainit.h"
#include "../evaio.h"
#include "../evabufferstream.h"
#include <string.h>

static guint error_count = 0;
static void
increment_error_counter (void)
{
  error_count++;
}

int main (int argc, char **argv)
{
  GError *error = NULL;
  EvaBufferStream *stream;
  eva_init (&argc, &argv, NULL);

  stream = eva_buffer_stream_new ();
  g_signal_connect (stream, "on-error", increment_error_counter, NULL);
  g_assert (eva_io_get_is_readable (stream));
  g_assert (eva_io_get_is_writable (stream));
  error_count = 0;
  eva_io_set_error (EVA_IO (stream), EVA_IO_ERROR_WRITE,
		    EVA_ERROR_BAD_FORMAT,
		    "hello, this is an error, %d", 16);
  g_assert (error_count == 1);
  error = EVA_IO (stream)->error;
  g_assert (error != NULL);
  g_assert (strstr (error->message, "error, 16") != NULL);
  g_assert (EVA_IO (stream)->error_cause == EVA_IO_ERROR_WRITE);
  g_assert (!eva_io_get_is_readable (stream));
  g_assert (!eva_io_get_is_writable (stream));
  g_object_unref (stream);

  stream = eva_buffer_stream_new ();
  eva_io_clear_shutdown_on_error (stream);
  g_signal_connect (stream, "on-error", increment_error_counter, NULL);
  g_assert (eva_io_get_is_readable (stream));
  g_assert (eva_io_get_is_writable (stream));
  error_count = 0;
  error = g_error_new (EVA_G_ERROR_DOMAIN,
		       EVA_ERROR_BAD_FORMAT,
		       "whatever");
  eva_io_set_gerror (EVA_IO (stream), EVA_IO_ERROR_WRITE, error);
  g_assert (error_count == 1);
  g_assert (EVA_IO (stream)->error == error);
  g_assert (strcmp (error->message, "whatever") == 0);
  g_assert (EVA_IO (stream)->error_cause == EVA_IO_ERROR_WRITE);
  g_assert (eva_io_get_is_readable (stream));
  g_assert (eva_io_get_is_writable (stream));
  g_object_unref (stream);

  return 0;
}
