#include "../evainit.h"
#include "../evastreamexternal.h"
#include "../evabufferstream.h"
#include <string.h>

int
main (int argc, char **argv)
{
  EvaBufferStream *input_buffer;
  EvaBufferStream *output_buffer;
  EvaStream *external;
  const char *args[10];
  const char *desired_output;
  char tmp[1024];

  eva_init_without_threads (&argc, &argv);

  /* filter some text through 'grep cat' */
  input_buffer = eva_buffer_stream_new();
  output_buffer = eva_buffer_stream_new();
  eva_buffer_append_string (eva_buffer_stream_peek_read_buffer (input_buffer),
                            "cat\n"
			    "dog\n"
			    "kitty-cat\n"
			    "elephant\n");
  eva_buffer_stream_read_buffer_changed (input_buffer);
  eva_buffer_stream_read_shutdown (input_buffer);
  args[0] = "grep";
  args[1] = "cat";
  args[2] = NULL;
  external = eva_stream_external_new (EVA_STREAM_EXTERNAL_SEARCH_PATH,
				      NULL, NULL,	/* in/out redirect */
				      NULL, NULL, NULL, /* termination */
				      "grep", args,	/* args */
				      NULL,		/* environment */
				      NULL		/* error */
				     );
  eva_stream_attach (EVA_STREAM (input_buffer), external, NULL);
  eva_stream_attach (external, EVA_STREAM (output_buffer), NULL);
  desired_output = "cat\nkitty-cat\n";
  while (eva_buffer_stream_peek_write_buffer (output_buffer)->size < strlen (desired_output))
    eva_main_loop_run (eva_main_loop_default (), -1, NULL);
  g_assert (eva_buffer_stream_peek_write_buffer (output_buffer)->size == strlen (desired_output));
  eva_buffer_read (eva_buffer_stream_peek_write_buffer (output_buffer), tmp, sizeof (tmp));
  g_assert (memcmp (tmp, desired_output, strlen (desired_output)) == 0);
  eva_buffer_stream_write_buffer_changed (output_buffer);
  
  return 0;
}
