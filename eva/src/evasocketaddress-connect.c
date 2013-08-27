#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include "evasocketaddress.h"
#include "evaerrno.h"
#include "evaerror.h"
#include "evamacros.h"
#include "evaghelpers.h"
#include "evautils.h"

/* implementation of the stuff which creates connecting file-descriptor,
 * and also finishing the connection if the connection process blocks.
 */

/* --- connecting to a socket-address --- */

/**
 * eva_socket_address_connect_fd:
 * @address: the address to connect to.
 * @is_connected: whether the connection succeeded completely.
 * @error: an optional error return.
 *
 * Begin connecting to a location by address.
 *
 * If the connection is fully established before returning to
 * the caller, then *@is_connected will be set to TRUE
 * and a non-negative file descriptor will be returned.
 *
 * Sometimes connections only partially succeed,
 * in which case *@is_connected will be set to FALSE,
 * and you must call eva_socket_address_finish_fd() whenever the
 * file-description polls ready to input or output.
 *
 * If the connect fails immediately, -1 will be returned
 * and *@error will be set if @error is non-NULL.
 *
 * returns: the connecting or connected file-descriptor, or -1 on error.
 */
int
eva_socket_address_connect_fd    (GskSocketAddress *address,
				  gboolean         *is_connected,
				  GError          **error)
{
  guint size = eva_socket_address_sizeof_native (address);
  struct sockaddr *addr = alloca (size);
  int fd;
  if (!eva_socket_address_to_native (address, addr, error))
    return -1;
retry:
  fd = socket (eva_socket_address_protocol_family (address), SOCK_STREAM, 0);
  if (fd < 0)
    {
      if (eva_errno_is_ignorable (errno))
        goto retry;
      eva_errno_fd_creation_failed ();
      if (error != NULL && *error == NULL)
	{
	  char *addr_str = eva_socket_address_to_string (address);
	  int e = errno;
	  *error = g_error_new (EVA_G_ERROR_DOMAIN,
			 eva_error_code_from_errno (e),
			 _("socket(2) failed when creating a connection (%s): %s"),
			 addr_str,
			 g_strerror (e));
	  g_free (addr_str);
	}
      return -1;
    }

  eva_fd_set_nonblocking (fd);
  eva_fd_set_close_on_exec (fd, TRUE);

  if (connect (fd, addr, size) < 0)
    {
      int e = errno;
      if (e == EINPROGRESS)
	{
	  /* neither success nor failure: the connection process
	   * itself is blocking */
	  *is_connected = FALSE;
	  return fd;
	}

      if (error != NULL && *error == NULL)
	{
	  char *addr_str = eva_socket_address_to_string (address);
	  *error = g_error_new (EVA_G_ERROR_DOMAIN,
			 eva_error_code_from_errno (e),
			 _("connect(2) failed when creating a connection (%s): %s"),
			 addr_str,
			 g_strerror (e));
	  g_free (addr_str);
	}
      close (fd);
      return -1;
    }

  *is_connected = TRUE;
  return fd;
}

/**
 * eva_socket_address_finish_fd:
 * @fd: a file descriptor which may be done connecting.
 * @error: an optional error return.
 *
 * Finish connecting a partially connected file-descriptor.
 *
 * returns: TRUE if the connection is now established,
 * otherwise it returns FALSE and will set *@error if an error occurred.
 */
gboolean
eva_socket_address_finish_fd     (int               fd,
				  GError          **error)
{
  int err = eva_errno_from_fd (fd);
  if (err == 0)
    return TRUE;

  if (!eva_errno_is_ignorable (err))
    g_set_error (error, EVA_G_ERROR_DOMAIN,
		 eva_error_code_from_errno (err),
		 _("error finishing connection: %s"),
		 g_strerror (err));
  return FALSE;
}
