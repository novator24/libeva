#include "../eva.h"
#include <stdlib.h>


static gboolean shutdown_and_unref (gpointer data)
{
  eva_io_shutdown (EVA_IO (data), NULL);
  g_object_unref (data);
  return FALSE;
}
static gboolean
handle_accept (GskStream    *stream,
               gpointer      data,
               GError      **error)
{
  eva_main_loop_add_timer (eva_main_loop_default (),
                           shutdown_and_unref,
                           g_object_ref (stream),
                           NULL,
                           3000, -1);
  return TRUE;
}

int
main(int argc, char **argv)
{
  GskStreamListener *listener;
  unsigned port;
  GskSocketAddress *address;
  GError *error = NULL;

  eva_init_without_threads (&argc, &argv);
  if (argc != 2)
    eva_fatal_user_error ("usage: eva-connreset-daemon PORT");
  port = atoi (argv[1]);
  if (port <= 0)
    eva_fatal_user_error ("error parsing port from: %s", argv[1]);

  address = eva_socket_address_ipv4_new (eva_ipv4_ip_address_any, port);
  listener = eva_stream_listener_socket_new_bind (address, &error);
  if (listener == NULL)
    eva_fatal_user_error ("error binding to port %u: %s", port, error->message);
  eva_stream_listener_handle_accept (listener, handle_accept, NULL, NULL, NULL);
  return eva_main_run ();
}
