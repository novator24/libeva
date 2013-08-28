#include "../evapersistentconnection.h"
#include "../evainit.h"
#include "../evamain.h"
#include "../evastreamfd.h"
#include <unistd.h>
#include <stdlib.h>

static void handle_connect (void) { g_message ("connected"); }
static void handle_disconnect (void) { g_message ("disconnected"); }


#define RETRY_TIMEOUT   1000    /* retry every second */

int main(int argc, char **argv)
{
  EvaStream *stream;
  eva_init_without_threads (&argc, &argv);

  if (argc != 3)
    g_error ("usage: test-persistent-connection HOST PORT");

  stream = eva_persistent_connection_new_lookup (argv[1], atoi (argv[2]), RETRY_TIMEOUT);
  g_assert (stream != NULL);

  eva_stream_attach (eva_stream_fd_new_auto (STDIN_FILENO), stream, NULL);
  eva_stream_attach (stream, eva_stream_fd_new_auto (STDOUT_FILENO), NULL);
  g_signal_connect (stream, "handle-connected", G_CALLBACK (handle_connect), NULL);
  g_signal_connect (stream, "handle-disconnected", G_CALLBACK (handle_disconnect), NULL);
  return eva_main_run ();
}
