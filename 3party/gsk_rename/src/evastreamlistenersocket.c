#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>  /* for maybe_delete_stale_socket() below */
#include <unistd.h>
#include <errno.h>

#include "evastreamlistenersocket.h"
#include "evaerrno.h"
#include "evaerror.h"
#include "evaghelpers.h"
#include "evautils.h"
#include "evastreamfd.h"
#include "evatypes.h"
#include "evamacros.h"
#include "evadebug.h"
#include "debug.h"
#include "config.h"

/* Set to 1 to use glib-style file-descriptor handling. */
#define USE_GLIB_MAIN_LOOP		EVA_STREAM_LISTENER_SOCKET_USE_GLIB_MAIN_LOOP

static GObjectClass *parent_class = NULL;

enum
{
  PROP_0,

  /* Construct-only */
  PROP_FILE_DESCRIPTOR,
  PROP_LISTENING_ADDRESS,
  PROP_MAY_REUSE_ADDRESS,

  /* anytime before finalize, only for local addresses */
  PROP_UNLINK_WHEN_DONE
};

/* --- dealing with the main-loop --- */

static void
notify_error (EvaStreamListenerSocket *listener,
              int                      errno_value)
{
  GError *err = g_error_new (EVA_G_ERROR_DOMAIN,
                             eva_error_code_from_errno (errno_value),
                             "error on accepting-fd %d: %s",
                             listener->fd, g_strerror (errno_value));
  eva_stream_listener_notify_error (EVA_STREAM_LISTENER (listener), err);
}

static void
handle_input_event (EvaStreamListenerSocket *listener)
{
  EvaStreamListener *stream_listener = EVA_STREAM_LISTENER (listener);
  int fd = listener->fd;
  struct sockaddr addr;
  socklen_t addr_len = sizeof (addr);
  int accept_fd;
  EvaStream *stream;

  accept_fd = accept (fd, &addr, &addr_len);
  if (EVA_IS_DEBUGGING(FD))
    {
      char *addr_str;
      addr_str = eva_socket_address_to_string (listener->listening_address);
      g_message ("debug-fd: open(%d): accepted on address %s",
                 accept_fd, addr_str);
      g_free (addr_str);
    }
  if (accept_fd < 0)
    {
      int e = errno;
      if (eva_errno_is_ignorable (e))
        return;
      eva_errno_fd_creation_failed_errno (e);
      notify_error (listener, e);
    }
  else
    {
      EvaSocketAddress *remote_addr;
      eva_fd_set_close_on_exec (accept_fd, TRUE);
      eva_fd_set_nonblocking (accept_fd);
      stream = eva_stream_fd_new (accept_fd, EVA_STREAM_FD_FOR_NEW_SOCKET);
      remote_addr = eva_socket_address_from_native (&addr, addr_len);
      if (remote_addr != NULL)
        g_object_set_qdata_full (G_OBJECT (stream),
                                 EVA_SOCKET_ADDRESS_REMOTE_QUARK,
                                 remote_addr,
                                 g_object_unref);
      if (listener->listening_address != NULL)
        g_object_set_qdata_full (G_OBJECT (stream),
                                 EVA_SOCKET_ADDRESS_LOCAL_QUARK,
                                 g_object_ref (listener->listening_address),
                                 g_object_unref);
      eva_stream_listener_notify_accepted (stream_listener, stream);
    }
}


#if USE_GLIB_MAIN_LOOP
typedef struct _EvaStreamListenerSocketSource EvaStreamListenerSocketSource;
struct _EvaStreamListenerSocketSource
{
  GSource source;
  EvaStreamListenerSocket *listener;
};

static gboolean
eva_stream_listener_socket_source_prepare (GSource    *source,
				           gint       *timeout)
{
  return FALSE;
}

static gboolean
eva_stream_listener_socket_source_check (GSource    *source)
{
  EvaStreamListenerSocketSource *listener_source
    = (EvaStreamListenerSocketSource *) source;
  EvaStreamListenerSocket *listener = listener_source->listener;
  return (listener->poll_fd.revents & (G_IO_IN | G_IO_ERR)) != 0;
}

static gboolean
eva_stream_listener_socket_source_dispatch (GSource    *source,
				            GSourceFunc callback,
				            gpointer    user_data)
{
  EvaStreamListenerSocketSource *listener_source
    = (EvaStreamListenerSocketSource *) source;
  EvaStreamListenerSocket *listener = listener_source->listener;
  guint events = listener->poll_fd.revents;
  if (events & G_IO_ERR)
    handle_input_event (listener, TRUE);
  else if (events & G_IO_IN)
    handle_input_event (listener, FALSE);
  return TRUE;
}

static GSourceFuncs eva_stream_listener_socket_source_funcs =
{
  eva_stream_listener_socket_source_prepare,
  eva_stream_listener_socket_source_check,
  eva_stream_listener_socket_source_dispatch,
  NULL,					/* finalize */
  NULL,					/* closure-callback (reserved) */
  NULL					/* closure-marshal (reserved) */
};

static void
add_poll (EvaStreamListenerSocket *socket)
{
  GPollFD *poll_fd = &socket->poll_fd;
  EvaStreamListenerSocketSource *socket_source;
  g_return_if_fail (socket->fd >= 0);
  g_return_if_fail (socket->source == NULL);
  socket->source = g_source_new (&eva_stream_listener_socket_source_funcs,
				 sizeof (EvaStreamListenerSocketSource));
  socket_source = (EvaStreamListenerSocketSource *) socket->source;
  socket_source->listener = socket;
  poll_fd->fd = socket->fd;
  poll_fd->events = G_IO_IN | G_IO_ERR;
  g_source_add_poll (socket->source, poll_fd);
  g_source_attach (socket->source, g_main_context_default ());
}

static void
remove_poll (EvaStreamListenerSocket *socket)
{
  GSource *source = socket->source;
  g_return_if_fail (source != NULL);
  socket->source = NULL;
  g_source_destroy (source);
  socket->poll_fd.fd = -1;
}
#else	/* !USE_GLIB_MAIN_LOOP */
static gboolean
handle_fd_event (int fd, GIOCondition events, gpointer user_data)
{
  EvaStreamListenerSocket *listener = EVA_STREAM_LISTENER_SOCKET (user_data);
  g_return_val_if_fail (fd == listener->fd, TRUE);
  if (events & G_IO_ERR)
    notify_error (listener, eva_errno_from_fd (fd));
  else if (events & G_IO_IN)
    handle_input_event (listener);

  return TRUE;
}

static void
add_poll (EvaStreamListenerSocket *socket)
{
  socket->source = eva_main_loop_add_io (eva_main_loop_default (),
					 socket->fd,
					 G_IO_IN | G_IO_ERR,
				         handle_fd_event,
				         socket,
				         NULL);
}
static void
remove_poll (EvaStreamListenerSocket *socket)
{
  if (socket->source != NULL)
    {
      eva_source_remove (socket->source);
      socket->source = NULL;
    }
}
#endif	/* !USE_GLIB_MAIN_LOOP */

/* --- arguments --- */

/* note: you cannot set these arguments multiple times
   because they are construct-only parameters. */
static void
eva_stream_listener_socket_set_property (GObject         *object,
                                         guint            property_id,
                                         const GValue    *value,
                                         GParamSpec      *pspec)
{
  EvaStreamListenerSocket *socket = EVA_STREAM_LISTENER_SOCKET (object);
  switch (property_id)
    {
    case PROP_FILE_DESCRIPTOR:
      socket->fd = g_value_get_int (value);
      return;

    case PROP_LISTENING_ADDRESS:
      if (socket->listening_address != NULL)
        g_object_unref (socket->listening_address);
      socket->listening_address = EVA_SOCKET_ADDRESS (g_value_dup_object (value));
      if (socket->unlink_when_done
        && (socket->listening_address == NULL
            || !EVA_IS_SOCKET_ADDRESS_LOCAL (socket->listening_address)))
        socket->unlink_when_done = 0;
      return;

    case PROP_MAY_REUSE_ADDRESS:
      socket->may_reuse_address = g_value_get_boolean (value);
      return;

    case PROP_UNLINK_WHEN_DONE:
      if (g_value_get_boolean (value))
        {
          if (socket->listening_address == NULL
           || EVA_IS_SOCKET_ADDRESS_LOCAL (socket->listening_address))
            socket->unlink_when_done = 1;
        }
      else
        socket->unlink_when_done = 0;
      return;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
    }
}

static void
eva_stream_listener_socket_get_property (GObject        *object,
                                         guint           property_id,
                                         GValue         *value,
                                         GParamSpec     *pspec)
{
  EvaStreamListenerSocket *socket = EVA_STREAM_LISTENER_SOCKET (object);
  switch (property_id)
    {
    case PROP_FILE_DESCRIPTOR:
      g_value_set_int (value, socket->fd);
      return;

    case PROP_LISTENING_ADDRESS:
      if (socket->listening_address != NULL)
	g_value_set_object (value, socket->listening_address);
      return;

    case PROP_MAY_REUSE_ADDRESS:
      g_value_set_boolean (value, socket->may_reuse_address);
      return;

    case PROP_UNLINK_WHEN_DONE:
      g_value_set_boolean (value, socket->unlink_when_done);
      return;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
    }
}

/* this function is for handling the common problem 
   that we bind over-and-over again to the same
   unix path.

   ideally, you'd think the OS's SO_REUSEADDR flag would
   cause this to happen, but it doesn't,
   at least on my linux 2.6 box.

   in fact, we really need a way to test without
   actually connecting to the remote server,
   which might annoy it.

   XXX: we should survey what others do here... like x-windows...
 */
void
_eva_socket_address_local_maybe_delete_stale_socket (EvaSocketAddress *local_socket)
{
  const char *path = EVA_SOCKET_ADDRESS_LOCAL (local_socket)->path;
  gboolean is_connected;
  int fd;
  struct stat statbuf;
  GError *error = NULL;
  if (stat (path, &statbuf) < 0)
    return;
  if (!S_ISSOCK (statbuf.st_mode))
    {
      g_warning ("%s existed but was not a socket", path);
      return;
    }
  fd = eva_socket_address_connect_fd (local_socket, &is_connected, &error);
  if (fd >= 0)
    {
      /* this is probably a protocol violation, and might kinda
         piss off the other server! */
      close(fd);
      g_warning ("server on %s appears to be running", path);
      return;
    }

  /* ok, we should delete the stale socket */
  g_clear_error (&error);
  if (unlink (path) < 0)
    g_warning ("unable to delete %s: %s", path, g_strerror(errno));
}


static gboolean
try_init_fd (EvaStreamListenerSocket *listener_socket)
{
  size_t sizeof_addr;
  struct sockaddr *addr;
  int fd;
  int may_reuse_address = listener_socket->may_reuse_address ? 1 : 0;
  EvaSocketAddress *address = listener_socket->listening_address;
  EvaStreamListener *listener = EVA_STREAM_LISTENER (listener_socket);
  if (address == NULL)
    {
      eva_stream_listener_notify_error (listener,
      	g_error_new (EVA_G_ERROR_DOMAIN,
		     EVA_ERROR_INVALID_ARGUMENT,
		     _("must either get listening-fd or socket-address")));
      return FALSE;
    }

  sizeof_addr = eva_socket_address_sizeof_native (address);
  addr = alloca (sizeof_addr);

  {
    GError *error = NULL;
    if (!eva_socket_address_to_native (address, addr, &error))
      {
        if (error)
          eva_stream_listener_notify_error (listener, error);
        return FALSE;
      }
  }

  fd = socket (eva_socket_address_protocol_family (address), SOCK_STREAM, 0);
  if (fd < 0)
    {
      char *addr_str = eva_socket_address_to_string (address);
      int e = errno;
      eva_stream_listener_notify_error (listener,
        g_error_new (EVA_G_ERROR_DOMAIN,
                     eva_error_code_from_errno (e),
                     _("socket(2) failed when creating a listener (%s): %s"),
                     addr_str,
                     g_strerror (e)));
      g_free (addr_str);
      return FALSE;
    }
  eva_fd_set_close_on_exec (fd, TRUE);

  if (setsockopt (fd, SOL_SOCKET, SO_REUSEADDR,
		  &may_reuse_address, sizeof (may_reuse_address)) < 0)
    {
      /* XXX: we really should not even try this
	 on some address families. (ie AF_UNIX). */
      char *addr_str = eva_socket_address_to_string (address);
      g_warning ("setting whether to reuse socket addresses failed: "
		 "address='%s', may-reuse=%s: %s",
		 addr_str, may_reuse_address ? "yes" : "no",
		 g_strerror (errno));
    }

  if (EVA_IS_SOCKET_ADDRESS_LOCAL (address) && may_reuse_address)
    _eva_socket_address_local_maybe_delete_stale_socket (address);

  if (bind (fd, addr, sizeof_addr) < 0)
    {
      char *addr_str = eva_socket_address_to_string (address);
      int e = errno;
      eva_stream_listener_notify_error (listener,
        g_error_new (EVA_G_ERROR_DOMAIN,
                     eva_error_code_from_errno (e),
                     _("bind(2) failed when creating a listener (%s): %s"),
                     addr_str,
                     g_strerror (e)));
      g_free (addr_str);
      return FALSE;
    }
  if (listen (fd, SOMAXCONN) < 0)
    {
      char *addr_str = eva_socket_address_to_string (address);
      int e = errno;
      eva_stream_listener_notify_error (listener,
        g_error_new (EVA_G_ERROR_DOMAIN,
                     eva_error_code_from_errno (e),
                     _("listen(2) failed when creating a listener (%s): %s"),
                     addr_str,
                     g_strerror (e)));
      g_free (addr_str);
      return FALSE;
    }
#if USE_GLIB_MAIN_LOOP
  listener_socket->poll_fd.fd = fd;
#endif
  listener_socket->fd = fd;

  if (EVA_IS_DEBUGGING(FD))
    {
      char *addr_str;
      addr_str = eva_socket_address_to_string (listener_socket->listening_address);
      g_message ("debug-fd: open(%d): listening on address %s", fd, addr_str);
      g_free (addr_str);
    }
  return TRUE;
}

static GObject *
eva_stream_listener_socket_constructor (GType                  type,
                                        guint                  n_properties,
                                        GObjectConstructParam *properties)
{
  GObject *rv = parent_class->constructor (type, n_properties, properties);
  EvaStreamListenerSocket *socket = EVA_STREAM_LISTENER_SOCKET (rv);
  g_assert (socket->source == NULL);
  if (socket->fd < 0)
    {
      if (!try_init_fd (socket))
	return rv;
    }
  if (socket->fd >= 0)
    eva_fd_set_nonblocking (socket->fd);
  add_poll (socket);
  return rv;
}

static void
eva_stream_listener_socket_finalize (GObject *object)
{
  EvaStreamListenerSocket *socket = EVA_STREAM_LISTENER_SOCKET (object);
  EvaSocketAddress *address = socket->listening_address;
  remove_poll (socket);
  if (address != NULL)
    {
      if (socket->unlink_when_done
       && EVA_IS_SOCKET_ADDRESS_LOCAL (address))
        {
          EvaSocketAddressLocal *local = EVA_SOCKET_ADDRESS_LOCAL (address);
          const char *filename = local->path;
          unlink (filename);
        }
      g_object_unref (socket->listening_address);
    }
  if (socket->fd >= 0)
    {
      if (EVA_IS_DEBUGGING(FD))
        g_message ("debug-fd: close(%d): was listening", socket->fd);
      close (socket->fd);
      socket->fd = -1;
    }
  (*parent_class->finalize) (object);
}

/* --- functions --- */
static void
eva_stream_listener_socket_init (EvaStreamListenerSocket *socket)
{
  socket->fd = -1;
}

static void
eva_stream_listener_socket_class_init (EvaStreamListenerSocketClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GParamSpec *pspec;

  parent_class = g_type_class_peek_parent (class);

  object_class->get_property = eva_stream_listener_socket_get_property;
  object_class->set_property = eva_stream_listener_socket_set_property;
  object_class->constructor = eva_stream_listener_socket_constructor;
  object_class->finalize = eva_stream_listener_socket_finalize;

  pspec = eva_param_spec_fd ("file-descriptor",
			     _("File Descriptor"),
			     _("whence to accept new connections on"),
			     G_PARAM_CONSTRUCT_ONLY
			     | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_FILE_DESCRIPTOR, pspec);

  pspec = g_param_spec_object ("listening-address",
			       _("Listening Address"),
			       _("The name others will use to connect"),
			       EVA_TYPE_SOCKET_ADDRESS,
			       G_PARAM_CONSTRUCT_ONLY
			       | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_LISTENING_ADDRESS, pspec);

  pspec = g_param_spec_boolean ("may-reuse-address",
			       _("May Reuse Address"),
			       _("Whether a listening address may be reused as soon as it's closed"),
			       TRUE,
			       G_PARAM_CONSTRUCT_ONLY
			       | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_MAY_REUSE_ADDRESS, pspec);

  pspec = g_param_spec_boolean ("unlink-when-done",
                                _("Unlink when done"),
                                _("Unlink the file from the file-system (only for local socket-listeners)"),
			       FALSE,
			       G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_UNLINK_WHEN_DONE, pspec);
}

GType eva_stream_listener_socket_get_type()
{
  static GType stream_listener_socket_type = 0;
  if (!stream_listener_socket_type)
    {
      static const GTypeInfo stream_listener_socket_info =
      {
        sizeof(EvaStreamListenerSocketClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) eva_stream_listener_socket_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (EvaStreamListenerSocket),
        0,              /* n_preallocs */
        (GInstanceInitFunc) eva_stream_listener_socket_init,
	NULL		/* value_table */
      };
      GType parent = EVA_TYPE_STREAM_LISTENER;
      stream_listener_socket_type = g_type_register_static (parent,
                                                  "EvaStreamListenerSocket",
                                                  &stream_listener_socket_info,
						  0);
    }
  return stream_listener_socket_type;
}

/**
 * eva_stream_listener_socket_new_bind_full:
 * @address: the address that the listener should be bound to.
 * @flags: flags controlling the operation of the listener.
 * @error: optional location to store error at.
 *
 * Create a new listener bound to a specific socket-address.
 *
 * If @flags suggests @EVA_SOCKET_LISTENER_STREAM_DONT_REUSE_ADDRESS,
 * then the usual timeout rules about how often a port
 * may be bound are ignored.  This only really affects
 * TCP/IP socket listeners.
 *
 * returns: the newly created stream-listener.
 */
EvaStreamListener *
eva_stream_listener_socket_new_bind_full (EvaSocketAddress   *address,
				     EvaStreamListenerSocketFlags flags,
			             GError            **error)
{
  gboolean may_reuse_addr = (flags & EVA_STREAM_LISTENER_SOCKET_DONT_REUSE_ADDRESS) ? 0 : 1;
  GObject *rv = g_object_new (EVA_TYPE_STREAM_LISTENER_SOCKET,
		              "listening-address", address,
			      "may-reuse-address", may_reuse_addr,
			      NULL);
  EvaStreamListener *listener = EVA_STREAM_LISTENER (rv);
  if (listener->last_error != NULL)
    {
      if (error != NULL)
	{
	  /* HACK */
	  if (*error)
	    g_error_free (*error);
	  *error = listener->last_error;
	  listener->last_error = NULL;
	}
      g_object_unref (rv);
      return NULL;
    }
  return listener;
}

/**
 * eva_stream_listener_socket_new_bind:
 * @address: the address that the listener should be bound to.
 * @error: optional location to store error at.
 *
 * Create a new listener bound to a specific socket-address.
 *
 * returns: the newly created stream-listener.
 */
EvaStreamListener *
eva_stream_listener_socket_new_bind (EvaSocketAddress   *address,
			             GError            **error)
{
  return eva_stream_listener_socket_new_bind_full (address, 0, error);
}

/**
 * eva_stream_listener_socket_new_from_fd
 *
 * @fd: The bound socket.
 * @error: optional location to store error at.
 * @returns: The newly created stream-listener.
 *
 * Create a new listener for an already bound socket.
 */
EvaStreamListener *
eva_stream_listener_socket_new_from_fd (gint     fd,
                                        GError **error)
{
  EvaSocketAddress *address;
  EvaStreamListener *rv;
  struct sockaddr sock_addr;
  socklen_t sock_addr_len;

  sock_addr_len = sizeof (struct sockaddr);
  if (getsockname (fd, &sock_addr, &sock_addr_len) != 0)
    {
      int e = errno;
      *error = g_error_new (EVA_G_ERROR_DOMAIN, eva_error_code_from_errno (e),
                            "error on getsockname %d: %s", fd,
                            g_strerror (e));
      return NULL;
    }

  /* Create the EVA address structure */
  address = eva_socket_address_from_native (&sock_addr, sock_addr_len);
  if (address != NULL)
    {
      rv = g_object_new (EVA_TYPE_STREAM_LISTENER_SOCKET,
                         "listening-address", address,
                         "file-descriptor", fd,
                         NULL);
      g_object_unref (address);
    }
  else
    {
      rv = g_object_new (EVA_TYPE_STREAM_LISTENER_SOCKET,
                         "file-descriptor", fd,
                         NULL);
    }

  return rv;
}


/**
 * eva_stream_listener_socket_set_backlog:
 * @lis: the listener whose backlog quota should be affected.
 * @backlog: the number of incoming connections to accept before
 * refusing them.
 *
 * Set the number of incoming connections that can
 * be accepted before they are rejected outright.
 */
void    eva_stream_listener_socket_set_backlog (EvaStreamListenerSocket *lis,
						guint             backlog)
{
  listen (lis->fd, backlog);
}
