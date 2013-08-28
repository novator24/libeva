#include "evastreamclient.h"

/**
 * eva_stream_new_connecting:
 * @address: the address to connect to.
 * @error: optional place to put an error response.
 *
 * Create a stream connecting to a given address.
 *
 * returns: the newly created stream.
 */
EvaStream *
eva_stream_new_connecting (EvaSocketAddress  *address,
			   GError           **error)
{
  gboolean is_connected;
  int fd;

  /* symbolic addresses are handled internally to EvaStreamFd */
  if (EVA_IS_SOCKET_ADDRESS_SYMBOLIC (address))
    return eva_stream_fd_new_from_symbolic_address (EVA_SOCKET_ADDRESS_SYMBOLIC (address), error);

  /* try connecting to the (presumably native) address */
  fd = eva_socket_address_connect_fd (address, &is_connected, error);
  if (fd < 0)
    return NULL;

  if (is_connected)
    {
      /* finished connecting */
      return eva_stream_fd_new (fd, EVA_STREAM_FD_FOR_NEW_SOCKET);
    }
  else
    {
      /* connecting still pending */
      return eva_stream_fd_new_connecting (fd);
    }
}
