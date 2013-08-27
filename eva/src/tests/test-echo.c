#include <stdio.h>
#include <stdlib.h>
#include "../evastreamlistenersocket.h"
#include "../evadebug.h"
#include "../evainit.h"
#include "../evamain.h"

static gboolean
handle_on_accept (EvaStream         *stream,
		  gpointer           data,
		  GError           **error)
{
  GError *e = NULL;
  eva_stream_attach (stream, stream, &e);
  if (e != NULL)
    g_error ("eva_stream_attach: %s", e->message);
  g_object_unref (stream);
  return TRUE;
}

static void
handle_errors (GError     *error,
	       gpointer    data)
{
  g_error ("error accepting new socket: %s", error->message);
}

int main (int argc, char **argv)
{
  EvaStreamListener *listener;
  EvaSocketAddress *bind_addr;
  GError *error = NULL;
  eva_init_without_threads (&argc, &argv);

  if (argc != 2)
    g_error ("%s requires exactly 1 argument, tcp port number", argv[0]);

  bind_addr = eva_socket_address_ipv4_localhost (atoi (argv[1]));
  listener = eva_stream_listener_socket_new_bind (bind_addr, &error);
  if (error != NULL)
    g_error ("eva_stream_listener_tcp_unix failed: %s", error->message);
  g_assert (listener != NULL);

  eva_stream_listener_handle_accept (listener,
				     handle_on_accept,
				     handle_errors,
				     NULL,		/* data */
				     NULL);		/* destroy */

  return eva_main_run ();
}
