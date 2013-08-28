#include "../evabufferstream.h"
#include "../evastreamfd.h"
#include "../evainit.h"
#include <string.h>

int main (int argc, char **argv)
{
  EvaStream *read_end;
  EvaStream *write_end;
  EvaBufferStream *memory_input;
  EvaBufferStream *memory_output;
  GError *error = NULL;
  EvaMainLoop *loop;
  EvaBuffer *output_buffer;
  char buf[6];

  eva_init_without_threads (&argc, &argv);
  loop = eva_main_loop_default ();

  memory_input = eva_buffer_stream_new ();
  memory_output = eva_buffer_stream_new ();
  if (!eva_stream_fd_pipe (&read_end, &write_end, &error))
    g_error("error creating pipe: %s", error->message);
  eva_stream_attach (EVA_STREAM (memory_input), write_end, &error);
  g_assert (error == NULL);
  eva_stream_attach (read_end, EVA_STREAM (memory_output), &error);
  g_assert (error == NULL);
  eva_buffer_append_string (eva_buffer_stream_peek_read_buffer (memory_input), "hi mom");
  eva_buffer_stream_read_buffer_changed (memory_input);
  output_buffer = eva_buffer_stream_peek_write_buffer (memory_output);

  while (output_buffer->size < 6)
    eva_main_loop_run (loop, -1, NULL);
  g_assert (output_buffer->size == 6);
  eva_buffer_read (output_buffer, buf, 6);
  g_assert (memcmp (buf, "hi mom", 6) == 0);
  return 0;
}
