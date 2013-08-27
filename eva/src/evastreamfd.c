#include "config.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "evaghelpers.h"
#include "evautils.h"
#include "evastreamfd.h"
#include "evaerror.h"
#include "evatypes.h"
#include "evaerrno.h"
#include "evafork.h"
#include "evadebug.h"
#include "debug.h"
#include "evamacros.h"

/* Hmm: should be removed.  required for eva_socket_address_finish_fd() */
#include "evasocketaddress.h"

#if 0		/* is debugging on? */
#define DEBUG	g_message
#else
#define DEBUG(args...)
#endif

#define USE_GLIB_MAIN_LOOP	EVA_STREAM_FD_USE_GLIB_MAIN_LOOP

#define G_IO_CONNECT		(G_IO_IN | G_IO_OUT)


enum
{
  PROP_0,

  /* construct-only properties */
  PROP_FILE_DESCRIPTOR,
  PROP_IS_POLLABLE,
  PROP_IS_READABLE,
  PROP_IS_WRITABLE
};

typedef struct _EvaStreamFdPrivate EvaStreamFdPrivate;
struct _EvaStreamFdPrivate
{
  EvaSocketAddressSymbolic *symbolic;
  gpointer name_resolver;
};
#define GET_PRIVATE(stream_fd) \
  G_TYPE_INSTANCE_GET_PRIVATE(stream_fd, EVA_TYPE_STREAM_FD, EvaStreamFdPrivate)


static GObjectClass *parent_class = NULL;

static void
set_events (EvaStreamFd *stream_fd, GIOCondition events)
{
#if USE_GLIB_MAIN_LOOP
  stream_fd->poll_fd.events = events;
#else
  if (stream_fd->source != NULL)
    eva_source_adjust_io (stream_fd->source, events);
#endif
}

static void
handle_stream_fd_events (EvaStreamFd *stream_fd,
			 GIOCondition events)
{
  if (eva_stream_get_is_connecting (stream_fd))
    {
      GError *error = NULL;
      DEBUG ("eva_stream_fd_source_dispatch: eva_stream=IS-CONNECTING");
      if (events == 0)
	return;
      /* XXX: this function should be renamed or virtualized or something */
      if (!eva_socket_address_finish_fd (stream_fd->fd, &error))
	{
	  if (error)
	    {
	      DEBUG ("handle_stream_fd_events: %s", error->message);
	      set_events (stream_fd, 0);
	      eva_io_set_gerror (EVA_IO (stream_fd), EVA_IO_ERROR_CONNECT, error);

	      eva_io_notify_shutdown (EVA_IO (stream_fd));
	      return;
	    }
	  /* not done connecting yet: keep trying */
	  return;
	}
      DEBUG ("handle_stream_fd_events: connected successfully");
      set_events (stream_fd, stream_fd->post_connecting_events);
      eva_io_notify_connected (EVA_IO (stream_fd));
      return;
    }
  DEBUG ("eva_stream_fd_source_dispatch: revents=%d", events);
  if ((events & G_IO_IN) != 0
   && eva_io_get_is_readable (stream_fd))
    eva_io_notify_ready_to_read (EVA_IO (stream_fd));
  if ((events & G_IO_OUT) == G_IO_OUT
   && eva_io_get_is_writable (stream_fd))
    eva_io_notify_ready_to_write (EVA_IO (stream_fd));
  if ((events & G_IO_HUP) == G_IO_HUP)
    {
      if (eva_io_get_is_readable (stream_fd))
	eva_io_notify_read_shutdown (stream_fd);
      if (eva_io_get_is_writable (stream_fd))
	eva_io_notify_write_shutdown (stream_fd);
    }
  else if ((events & G_IO_ERR) == G_IO_ERR)
    {
      int e = eva_errno_from_fd (stream_fd->fd);
      EvaErrorCode code = eva_error_code_from_errno (e);
      eva_io_set_error (EVA_IO (stream_fd),
			EVA_IO_ERROR_POLL,
			code,
			"error polling file description %d: %s",
			stream_fd->fd,
			g_strerror (e));
    }
}


#if USE_GLIB_MAIN_LOOP
typedef struct _EvaStreamFdSource EvaStreamFdSource;
struct _EvaStreamFdSource
{
  GSource base;
  EvaStreamFd *stream_fd;
};

static gboolean
eva_stream_fd_source_prepare (GSource    *source,
			      gint       *timeout)
{
  DEBUG ("eva_stream_fd_source_prepare: events=%d", ((EvaStreamFdSource*)source)->stream_fd->poll_fd.events);
  return FALSE;
}

static gboolean
eva_stream_fd_source_check    (GSource    *source)
{
  EvaStreamFdSource *fd_source = (EvaStreamFdSource *) source;
  DEBUG ("eva_stream_fd_source_check: events=%d, revents=%d", fd_source->stream_fd->poll_fd.events,fd_source->stream_fd->poll_fd.revents);
  return fd_source->stream_fd->poll_fd.revents != 0;
}

static gboolean
eva_stream_fd_source_dispatch (GSource    *source,
			       GSourceFunc callback,
			       gpointer    user_data)
{
  EvaStreamFdSource *fd_source = (EvaStreamFdSource *) source;
  EvaStreamFd *stream_fd = fd_source->stream_fd;
  g_object_ref (stream_fd);	/* XXX: maybe unnecessary (for destroy-in-destroy) */
  handle_stream_fd_events (stream_fd, stream_fd->poll_fd.revents);
  g_object_unref (stream_fd);
  return TRUE;
}

static GSourceFuncs eva_stream_fd_source_funcs =
{
  eva_stream_fd_source_prepare,
  eva_stream_fd_source_check,
  eva_stream_fd_source_dispatch,
  NULL,					/* finalize */
  NULL,					/* closure-callback (reserved) */
  NULL					/* closure-marshal (reserved) */
};

static gboolean
add_poll (EvaStreamFd *stream_fd)
{
  g_return_val_if_fail (stream_fd->source == NULL, FALSE);

  stream_fd->source = g_source_new (&eva_stream_fd_source_funcs,
				    sizeof (EvaStreamFdSource));
  fd_source = (EvaStreamFdSource *) stream_fd->source;
  fd_source->stream_fd = stream_fd;
  stream_fd->poll_fd.fd = stream_fd->fd;
  stream_fd->poll_fd.events = G_IO_HUP;
  g_source_add_poll (stream_fd->source, &stream_fd->poll_fd);
  g_source_attach (stream_fd->source, g_main_context_default ());
  return TRUE;
}

static void
remove_poll (EvaStreamFd *stream_fd)
{
  if (stream_fd->source != NULL)
    {
      g_source_destroy (stream_fd->source);
      g_source_unref (stream_fd->source);
      stream_fd->source = NULL;
    }
}
#else  /* !USE_GLIB_MAIN_LOOP */
static gboolean
handle_io_event (int fd, GIOCondition events, gpointer user_data)
{
  EvaStreamFd *stream_fd = EVA_STREAM_FD (user_data);
  g_return_val_if_fail (stream_fd->fd == fd, TRUE);
  g_object_ref (stream_fd);	/* XXX: maybe unnecessary (for destroy-in-destroy) */
  handle_stream_fd_events (stream_fd, events);
  g_object_unref (stream_fd);
  return TRUE;
}

static gboolean
add_poll (EvaStreamFd *stream_fd)
{
  if (stream_fd->is_pollable)
    {
      stream_fd->source = eva_main_loop_add_io (eva_main_loop_default (),
						stream_fd->fd,
						G_IO_HUP,	/* events */
						handle_io_event,
						stream_fd,
						NULL);
    }
  else
    {
      EvaIO *io = EVA_IO (stream_fd);
      if (eva_io_get_is_readable (io))
	eva_io_mark_idle_notify_read (io);
      if (eva_io_get_is_writable (io))
	eva_io_mark_idle_notify_write (io);
    }
  return TRUE;
}

static void
remove_poll (EvaStreamFd *stream_fd)
{
  if (stream_fd->is_pollable)
    {
      if (stream_fd->source != NULL)
	{
	  eva_source_remove (stream_fd->source);
	  stream_fd->source = NULL;
	}
    }
  else
    {
      EvaIO *io = EVA_IO (stream_fd);
      eva_io_clear_idle_notify_read (io);
      eva_io_clear_idle_notify_write (io);
    }
}

#endif  /* !USE_GLIB_MAIN_LOOP */
/* Note: don't bother checking 'is_readable/is_writable':
 *       evastream does that itself.
 */

static inline void
eva_stream_fd_set_poll_event  (EvaStreamFd   *stream_fd,
			       gushort        event_mask,
			       gboolean       do_poll)
{
  if (stream_fd->is_resolving_name
   || eva_io_get_is_connecting (stream_fd))
    {
      if (do_poll)
	stream_fd->post_connecting_events |= event_mask;
      else
	stream_fd->post_connecting_events &= ~event_mask;
    }
  else if (stream_fd->failed_name_resolution)
    {
      /* do nothing, about to shutdown */
    }
  else
    {
#if USE_GLIB_MAIN_LOOP
      if (do_poll)
	stream_fd->poll_fd.events |= event_mask;
      else
	stream_fd->poll_fd.events &= ~event_mask;
#else
      if (do_poll)
        eva_source_add_io_events (stream_fd->source, event_mask);
      else
        eva_source_remove_io_events (stream_fd->source, event_mask);
#endif
    }
}

static void
eva_stream_fd_set_poll_read   (EvaIO         *io,
			       gboolean       do_poll)
{
  EvaStreamFd *stream_fd = EVA_STREAM_FD (io);
  if (stream_fd->is_pollable)
    eva_stream_fd_set_poll_event (stream_fd, G_IO_IN, do_poll);
}

static void
eva_stream_fd_set_poll_write  (EvaIO         *io,
			       gboolean       do_poll)
{
  EvaStreamFd *stream_fd = EVA_STREAM_FD (io);
  if (stream_fd->is_pollable)
    eva_stream_fd_set_poll_event (stream_fd, G_IO_OUT, do_poll);
}

/* --- reading and writing --- */
static guint
eva_stream_fd_raw_read        (EvaStream     *stream,
			       gpointer       data,
			       guint          length,
			       GError       **error)
{
  EvaStreamFd *stream_fd = EVA_STREAM_FD (stream);
  int rv = read (stream_fd->fd, data, length);
  if (rv < 0)
    {
      gint e = errno;
      if (eva_errno_is_ignorable (e))
	return 0;
      if (e == ECONNRESET)
        {
          eva_io_notify_shutdown (EVA_IO (stream));
          return 0;
        }
      g_set_error (error, EVA_G_ERROR_DOMAIN,
		   eva_error_code_from_errno (e),
		   "error reading from fd %d: %s",
		   stream_fd->fd,
		   g_strerror (e));
      return 0;
    }
  if (rv == 0)
    {
      eva_io_notify_read_shutdown (EVA_IO (stream));
    }
  return rv;
}

static guint
eva_stream_fd_raw_write       (EvaStream     *stream,
			       gconstpointer  data,
			       guint          length,
			       GError       **error)
{
  EvaStreamFd *stream_fd = EVA_STREAM_FD (stream);
  int rv;
  if (stream_fd->fd == -1)
    return 0;
  rv = write (stream_fd->fd, data, length);
  if (rv < 0)
    {
      gint e = errno;
      if (eva_errno_is_ignorable (e))
	return 0;
      if (e == ECONNRESET)
        {
          eva_io_notify_shutdown (EVA_IO (stream));
          return 0;
        }
      g_set_error (error, EVA_G_ERROR_DOMAIN,
		   eva_error_code_from_errno (e),
		   "error writing to fd %d: %s",
		   stream_fd->fd,
		   g_strerror (e));
      //{char*t=eva_escape_memory(data,length); g_message ("errorr writing : data=%s",t);g_free(t);}
      eva_io_notify_shutdown (EVA_IO (stream_fd));
      return 0;
    }
  return rv;
}

static guint
eva_stream_fd_raw_read_buffer(EvaStream    *stream,
			      EvaBuffer     *buffer,
			      GError       **error)
{
  EvaStreamFd *stream_fd = EVA_STREAM_FD (stream);
  int rv;
  if (stream_fd->fd == -1)
    return 0;
  rv = eva_buffer_read_in_fd (buffer, stream_fd->fd);
  if (rv < 0)
    {
      gint e = errno;
      if (eva_errno_is_ignorable (e))
	return 0;
      g_set_error (error, EVA_G_ERROR_DOMAIN,
		   eva_error_code_from_errno (e),
		   "error reading into buffer from fd %d: %s",
		   stream_fd->fd,
		   g_strerror (e));
      eva_io_notify_shutdown (EVA_IO (stream));
      return 0;
    }
  if (rv == 0)
    eva_io_notify_read_shutdown (EVA_IO (stream));
  return (guint) rv;
}

static guint
eva_stream_fd_raw_write_buffer (EvaStream     *stream,
				EvaBuffer     *buffer,
			        GError       **error)
{
  EvaStreamFd *stream_fd = EVA_STREAM_FD (stream);
  int rv;
  if (stream_fd->fd == -1)
    return 0;
  rv = eva_buffer_writev (buffer, stream_fd->fd);
  if (rv < 0)
    {
      gint e = errno;
      if (eva_errno_is_ignorable (e))
	return 0;
      g_set_error (error, EVA_G_ERROR_DOMAIN,
		   eva_error_code_from_errno (e),
		   "error writing from buffer to fd %d: %s",
		   stream_fd->fd,
		   g_strerror (e));
      eva_io_notify_shutdown (EVA_IO (stream_fd));
      return 0;
    }
  return (guint) rv;
}

/* --- shutting-down and closing --- */
static void
eva_stream_fd_close (EvaIO         *io)
{
  EvaStreamFd *stream_fd = EVA_STREAM_FD (io);
  remove_poll (stream_fd);
  if (stream_fd->fd >= 0)
    {
      if (EVA_IS_DEBUGGING (FD))
        g_message ("debug-fd: close(%d): was a eva-stream", stream_fd->fd);
      close (stream_fd->fd);
      eva_fork_remove_cleanup_fd (stream_fd->fd);
      stream_fd->fd = -1;
      stream_fd->is_pollable = 0;
    }
}

static gboolean
eva_stream_fd_shutdown_read   (EvaIO         *io,
			       GError       **error)
{
  EvaStreamFd *stream_fd = EVA_STREAM_FD (io);
  if (stream_fd->is_resolving_name)
    {
      if (!eva_io_get_is_writable (io))
        {
          EvaStreamFdPrivate *priv = GET_PRIVATE (stream_fd);
          eva_socket_address_symbolic_cancel_resolution (priv->symbolic,
                                                         priv->name_resolver);
        }
    }
  else if (stream_fd->is_shutdownable)
    {
      if (shutdown (stream_fd->fd, SHUT_RD) < 0)
	{
	  int e = errno;

	  /* NOTE: there seems to be evidence that the other side
	     can close the connection before giving us
	     any HUP notification.  In this case, we get
	     ENOTCONN when we attempt the shutdown.
	     Just ignore that condition.

             TODO: find a real discussion of ENOTCONN */
	  if (e != ENOTCONN)
	    {
	      g_set_error (error, EVA_G_ERROR_DOMAIN,
			   eva_error_code_from_errno (e),
			   "error shutting down fd %d for reading: %s",
			   stream_fd->fd,
			   g_strerror (e));
	      return FALSE;
	    }
	}
    }
  else
    {
      if (!eva_io_get_is_writable (io))
	eva_stream_fd_close (io);
    }
  return TRUE;
}

static gboolean
eva_stream_fd_shutdown_write  (EvaIO         *io,
			       GError       **error)
{
  EvaStreamFd *stream_fd = EVA_STREAM_FD (io);
  if (stream_fd->is_resolving_name)
    {
      if (!eva_io_get_is_readable (io))
        {
          EvaStreamFdPrivate *priv = GET_PRIVATE (stream_fd);
          eva_socket_address_symbolic_cancel_resolution (priv->symbolic,
                                                         priv->name_resolver);
        }
    }
  else if (stream_fd->is_shutdownable)
    {
      if (shutdown (stream_fd->fd, SHUT_WR) < 0)
	{
	  int e = errno;
	  /* NOTE: there seems to be evidence that the other side
	     can close the connection before giving us
	     any HUP notification.  In this case, we get
	     ENOTCONN when we attempt the shutdown.
	     Just ignore that condition.

             TODO: find a real discussion of ENOTCONN */
	  if (e != ENOTCONN)
	    {
	      g_set_error (error, EVA_G_ERROR_DOMAIN,
			   eva_error_code_from_errno (e),
			   "error shutting down fd %d for writing: %s",
			   stream_fd->fd,
			   g_strerror (e));
	      return FALSE;
	    }
	}
    }
  else
    {
      if (!eva_io_get_is_readable (io))
	eva_stream_fd_close (io);
    }
  return TRUE;
}

/* --- arguments --- */
static void
eva_stream_fd_get_property (GObject        *object,
			    guint           property_id,
			    GValue         *value,
			    GParamSpec     *pspec)
{
  switch (property_id)
    {
    case PROP_FILE_DESCRIPTOR:
      g_value_set_int (value, EVA_STREAM_FD (object)->fd);
      break;
    case PROP_IS_POLLABLE:
      g_value_set_boolean (value, EVA_STREAM_FD (object)->is_pollable);
      break;
    case PROP_IS_READABLE:
      g_value_set_boolean (value, eva_io_get_is_readable (object));
      break;
    case PROP_IS_WRITABLE:
      g_value_set_boolean (value, eva_io_get_is_writable (object));
      break;
    }
}

static void
eva_stream_fd_set_property (GObject        *object,
			    guint           property_id,
			    const GValue   *value,
			    GParamSpec     *pspec)
{
  switch (property_id)
    {
    case PROP_FILE_DESCRIPTOR:
      {
	int fd = g_value_get_int (value);
	EvaStreamFd *stream_fd = EVA_STREAM_FD (object);
	if (stream_fd->fd >= 0)
	  eva_fork_remove_cleanup_fd (fd);
	if (fd >= 0)
	  eva_fork_add_cleanup_fd (fd);
	stream_fd->fd = fd;
	break;
      }
    case PROP_IS_POLLABLE:
      {
	EVA_STREAM_FD (object)->is_pollable = g_value_get_boolean (value);
	break;
      }
    case PROP_IS_WRITABLE:
      { 
	if (g_value_get_boolean (value))
	  eva_io_mark_is_writable (object);
	else
	  eva_io_clear_is_writable (object);
	break;
      }
    case PROP_IS_READABLE:
      {
	if (g_value_get_boolean (value))
	  eva_io_mark_is_readable (object);
	else
	  eva_io_clear_is_readable (object);
	break;
      }
    }
}

static void
eva_stream_fd_finalize (GObject *object)
{
  EvaStreamFd *stream_fd = EVA_STREAM_FD (object);
  if (stream_fd->is_resolving_name)
    {
      EvaStreamFdPrivate *priv = GET_PRIVATE (stream_fd);
      if (priv->name_resolver != NULL)
        {
          eva_socket_address_symbolic_cancel_resolution (priv->symbolic, priv->name_resolver);
          priv->name_resolver = NULL;
        }
      stream_fd->is_resolving_name = 0;
      g_object_unref (priv->symbolic);
      priv->symbolic = NULL;
    }
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static gboolean
eva_stream_fd_open (EvaIO     *io,
		    GError   **error)
{
  EvaStreamFd *stream_fd = EVA_STREAM_FD (io);
  if (stream_fd->fd < 0)
    return TRUE;                /* permit postponed fd assignment */
  return add_poll (stream_fd);
}

/* --- functions --- */
static void
eva_stream_fd_init (EvaStreamFd *stream_fd)
{
  stream_fd->fd = -1;
}

static void
eva_stream_fd_class_init (EvaStreamClass *class)
{
  GParamSpec *pspec;
  EvaIOClass *io_class = EVA_IO_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  parent_class = g_type_class_peek_parent (class);
  class->raw_read = eva_stream_fd_raw_read;
  class->raw_read_buffer = eva_stream_fd_raw_read_buffer;
  class->raw_write = eva_stream_fd_raw_write;
  class->raw_write_buffer = eva_stream_fd_raw_write_buffer;
  io_class->set_poll_read = eva_stream_fd_set_poll_read;
  io_class->set_poll_write = eva_stream_fd_set_poll_write;
  io_class->shutdown_read = eva_stream_fd_shutdown_read;
  io_class->shutdown_write = eva_stream_fd_shutdown_write;
  io_class->open = eva_stream_fd_open;
  io_class->close = eva_stream_fd_close;
  object_class->get_property = eva_stream_fd_get_property;
  object_class->set_property = eva_stream_fd_set_property;
  object_class->finalize = eva_stream_fd_finalize;

  g_type_class_add_private (class, sizeof (EvaStreamFdPrivate));

  pspec = eva_param_spec_fd ("file-descriptor",
			     _("File Descriptor"),
			     _("for reading and/or writing"),
                             G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_FILE_DESCRIPTOR, pspec);

  pspec = g_param_spec_boolean ("is-pollable",
			        _("Is Pollable"),
			        _("whether the file descriptor is pollable"),
				FALSE,
                                G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_IS_POLLABLE, pspec);
  pspec = g_param_spec_boolean ("is-readable",
				_("Is Readable"),
				_("is the FD readable"),
				FALSE,
				G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_IS_READABLE, pspec);
  pspec = g_param_spec_boolean ("is-writable",
				_("Is Writable"),
				_("is the FD writable"),
				FALSE,
				G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_IS_WRITABLE, pspec);
}

GType eva_stream_fd_get_type()
{
  static GType stream_fd_type = 0;
  if (!stream_fd_type)
    {
      static const GTypeInfo stream_fd_info =
      {
	sizeof(EvaStreamFdClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_stream_fd_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EvaStreamFd),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_stream_fd_init,
	NULL		/* value_table */
      };
      GType parent = EVA_TYPE_STREAM;
      stream_fd_type = g_type_register_static (parent,
					       "EvaStreamFd",
					       &stream_fd_info, 0);
    }
  return stream_fd_type;
}

EvaStreamFdFlags eva_stream_fd_flags_guess (gint            fd)
{
  struct stat stat_buf;
  EvaStreamFdFlags rv = 0;
  guint flags;
  if (fstat (fd, &stat_buf) < 0)
    {
      g_warning ("eva_stream_fd_flags_guess failed: fd=%d: %s",
		 fd, g_strerror (errno));
      return 0;
    }
  if (S_ISFIFO (stat_buf.st_mode)
   || S_ISSOCK (stat_buf.st_mode)
   || S_ISCHR (stat_buf.st_mode)
   || isatty (fd))
    rv |= EVA_STREAM_FD_IS_POLLABLE;
  if (S_ISSOCK (stat_buf.st_mode))
    rv |= EVA_STREAM_FD_IS_SHUTDOWNABLE;
  flags = fcntl (fd, F_GETFL);
  if ((flags & O_ACCMODE) == O_RDONLY)
    rv |= EVA_STREAM_FD_IS_READABLE;
  if ((flags & O_ACCMODE) == O_WRONLY)
    rv |= EVA_STREAM_FD_IS_WRITABLE;
  if ((flags & O_ACCMODE) == O_RDWR)
    rv |= EVA_STREAM_FD_IS_READABLE | EVA_STREAM_FD_IS_WRITABLE;
#if 0
  g_message ("eva_stream_fd_flags_guess: fd=%d: pollable=%s, shutdownable=%s, readable=%s, writeable=%s",
	     fd, 
	     rv & EVA_STREAM_FD_IS_POLLABLE ? "yes" : "no",
	     rv & EVA_STREAM_FD_IS_SHUTDOWNABLE ? "yes" : "no",
	     rv & EVA_STREAM_FD_IS_READABLE ? "yes" : "no",
	     rv & EVA_STREAM_FD_IS_WRITABLE ? "yes" : "no");
#endif
  return rv;
}

/**
 * eva_stream_fd_new_auto:
 * @fd: the file-descriptor to use as the basis for a stream.
 *
 * Try to guess the nature of the file-descriptor using fstat(),
 * isatty().
 *
 * returns: a new EvaStream which will free @fd when it is closed.
 */
EvaStream   *eva_stream_fd_new_auto        (gint            fd)
{
  EvaStreamFdFlags flags = eva_stream_fd_flags_guess (fd);
  if (flags == 0)
    return NULL;
  return eva_stream_fd_new (fd, flags);
}

/**
 * eva_stream_fd_new:
 * @fd: the raw file descriptor.
 * @flags: information about how to use the file descriptor.
 *
 * Create a new EvaStream based on an already open file descriptor.
 *
 * returns: a new EvaStream
 */
EvaStream *
eva_stream_fd_new (gint fd,
		   EvaStreamFdFlags flags)
{
  EvaStream *rv;
  EvaStreamFd *rv_fd;
  g_return_val_if_fail (fd >= 0, NULL);
  rv = g_object_new (EVA_TYPE_STREAM_FD, "file-descriptor", fd,
		     "is-pollable",
		     (flags & EVA_STREAM_FD_IS_POLLABLE) == EVA_STREAM_FD_IS_POLLABLE,
		     "is-readable",
		     (flags & EVA_STREAM_FD_IS_READABLE) == EVA_STREAM_FD_IS_READABLE,
		     "is-writable",
		     (flags & EVA_STREAM_FD_IS_WRITABLE) == EVA_STREAM_FD_IS_WRITABLE,
		     NULL);
  rv_fd = EVA_STREAM_FD (rv);
  if ((flags & EVA_STREAM_FD_IS_READABLE) == EVA_STREAM_FD_IS_READABLE)
    eva_stream_mark_is_readable (rv);
  if ((flags & EVA_STREAM_FD_IS_WRITABLE) == EVA_STREAM_FD_IS_WRITABLE)
    eva_stream_mark_is_writable (rv);
  rv_fd->is_shutdownable = (flags & EVA_STREAM_FD_IS_SHUTDOWNABLE) ? 1 : 0;

  return rv;
}

/**
 * eva_stream_fd_new_connecting:
 * @fd: the raw file descriptor.
 *
 * Create a new EvaStream based on a socket which is still in the process of connecting.
 *
 * returns: a new EvaStream
 */
EvaStream *
eva_stream_fd_new_connecting (gint fd)
{
  EvaStream *rv;
  EvaStreamFd *stream_fd;
  g_return_val_if_fail (fd >= 0, NULL);
  rv = g_object_new (EVA_TYPE_STREAM_FD,
		     "file-descriptor", fd,
		     "is-pollable", TRUE,
		     NULL);
  eva_stream_mark_is_connecting (rv);

  eva_stream_mark_is_readable (rv);
  eva_stream_mark_is_writable (rv);

  stream_fd = EVA_STREAM_FD (rv);
  stream_fd->is_shutdownable = 1;
  stream_fd->is_pollable = 1;
  set_events (stream_fd, G_IO_CONNECT);

  return rv;
}

/* address which requires a name resolution */
/**
 * eva_stream_fd_new_from_symbolic_address:
 * @symbolic: a symbolic address.  name resolution will begin immediately.
 * @error: optional error return value.
 *
 * Create a stream connecting to a symbolic socket-address.
 *
 * returns: a new EvaStream
 */
static void
done_resolving_name (EvaStreamFd *stream_fd)
{
  EvaStreamFdPrivate *priv = GET_PRIVATE (stream_fd);
  stream_fd->is_resolving_name = 0;
  priv->name_resolver = NULL;
  g_object_unref (priv->symbolic);
  priv->symbolic = NULL;
}

static void
handle_name_lookup_success  (EvaSocketAddressSymbolic *orig,
                             EvaSocketAddress         *resolved,
                             gpointer                  user_data)
{
  EvaStreamFd *stream_fd = EVA_STREAM_FD (user_data);
  GError *error = NULL;
  gboolean is_connected;

  g_object_ref (stream_fd);

  done_resolving_name (stream_fd);
  stream_fd->fd = eva_socket_address_connect_fd (resolved, &is_connected, &error);
  if (stream_fd->fd < 0)
    {
      eva_io_set_gerror (EVA_IO (stream_fd), EVA_IO_ERROR_CONNECT, error);
      eva_io_notify_shutdown (EVA_IO (stream_fd));
      goto cleanup;
    }
  stream_fd->is_shutdownable = 1;
  eva_fork_add_cleanup_fd (stream_fd->fd);
  add_poll (stream_fd);
  if (is_connected)
    {
      set_events (stream_fd, stream_fd->post_connecting_events);
    }
  else
    {
      set_events (stream_fd, G_IO_CONNECT);
      eva_stream_mark_is_connecting (stream_fd);
    }

cleanup:
  g_object_unref (stream_fd);
}

static void
handle_name_lookup_failure  (EvaSocketAddressSymbolic *orig,
                             const GError             *error,
                             gpointer                  user_data)
{
  EvaStreamFd *stream_fd = EVA_STREAM_FD (user_data);

  stream_fd->failed_name_resolution = 1;

  g_object_ref (stream_fd);
  done_resolving_name (stream_fd);
  eva_io_set_gerror (EVA_IO (stream_fd), EVA_IO_ERROR_CONNECT, g_error_copy (error));
  eva_io_notify_shutdown (EVA_IO (stream_fd));
  g_object_unref (stream_fd);
}

EvaStream *
eva_stream_fd_new_from_symbolic_address (EvaSocketAddressSymbolic *symbolic,
                                         GError                  **error)
{
  EvaStreamFd *stream_fd = g_object_new (EVA_TYPE_STREAM_FD, NULL);
  EvaStreamFdPrivate *priv = GET_PRIVATE (stream_fd);
  stream_fd->is_resolving_name = 1;
  stream_fd->is_pollable = 1;
  eva_stream_mark_is_readable (stream_fd);
  eva_stream_mark_is_writable (stream_fd);
  priv->symbolic = g_object_ref (symbolic);
  priv->name_resolver = eva_socket_address_symbolic_create_name_resolver (symbolic);
  eva_socket_address_symbolic_start_resolution (symbolic,
                                                priv->name_resolver,
                                                handle_name_lookup_success,
                                                handle_name_lookup_failure,
                                                stream_fd,
                                                NULL);
  return EVA_STREAM (stream_fd);
}


/* more constructors */

/**
 * eva_stream_fd_new_open:
 * @filename: file to open or create (depending on @open_flags)
 * @open_flags: same as the second argument to open(2).
 * @permission: permissions if creating a new file.
 * @error: optional error return value.
 *
 * Open a file as a #EvaStream; this interface strongly
 * reflects its underlying open(2) implementation.
 * Using eva_stream_fd_new_read_file()
 * and eva_stream_fd_new_write_file() may be more portable ultimately.
 *
 * returns: a new EvaStream
 */
EvaStream *
eva_stream_fd_new_open (const char     *filename,
			guint           open_flags,
			guint           permission,
			GError        **error)
{
  int fd = open (filename, open_flags, permission);
  EvaStream *rv;
  if (fd < 0)
    {
      int e = errno;
      eva_errno_fd_creation_failed_errno (e);
      g_set_error (error, EVA_G_ERROR_DOMAIN,
		   eva_error_code_from_errno (e),
		   _("error opening %s: %s"),
		   filename, g_strerror (e));
      return NULL;
    }
  eva_fd_set_close_on_exec (fd, TRUE);
  rv = eva_stream_fd_new_auto (fd);

  return rv;
}

/**
 * eva_stream_fd_new_read_file:
 * @filename: file to open readonly.
 * @error: optional error return value.
 *
 * Open a file for reading as a #EvaStream.
 * The stream is not writable.
 *
 * returns: a new EvaStream
 */
EvaStream *
eva_stream_fd_new_read_file   (const char     *filename,
			       GError        **error)
{
  return eva_stream_fd_new_open (filename, O_RDONLY, 0, error);
}

/**
 * eva_stream_fd_new_write_file:
 * @filename: file to open write-only.
 * @may_create: whether creating the filename is acceptable.
 * @should_truncate: whether an existing filename should be truncated.
 * @error: optional error return value.
 *
 * Open a file for writing as a #EvaStream.
 * The stream is not readable.
 *
 * returns: a new EvaStream
 */
EvaStream  *
eva_stream_fd_new_write_file  (const char     *filename,
			       gboolean        may_create,
			       gboolean        should_truncate,
			       GError        **error)
{
  guint flags = O_WRONLY | (may_create ? O_CREAT : 0)
              | (should_truncate ? O_TRUNC : 0);
  return eva_stream_fd_new_open (filename, flags, 0660, error);
}

/**
 * eva_stream_fd_new_create_file:
 * @filename: file to open write-only.
 * @may_exist: whether file may exist.
 * @error: optional error return value.
 *
 * Create a file for writing as a #EvaStream.
 * The stream is not readable.
 *
 * returns: a new EvaStream
 */
EvaStream *
eva_stream_fd_new_create_file (const char     *filename,
			       gboolean        may_exist,
			       GError        **error)
{
  guint flags = O_WRONLY | O_CREAT | (may_exist ? 0 : O_EXCL);
  return eva_stream_fd_new_open (filename, flags, 0660, error);
}

/**
 * eva_stream_fd_pipe:
 * @read_side_out: place to store a reference to a newly allocated readable stream-fd.
 * @write_side_out: place to store a reference to a newly allocated writable stream-fd.
 * @error: optional error return value.
 *
 * Call the pipe(2) system call to make a half-duplex connection
 * between two streams.
 * The newly allocated streams are returned.
 *
 * returns: whether the streams were allocated successfully.
 */
gboolean    eva_stream_fd_pipe     (EvaStream     **read_side_out,
                                    EvaStream     **write_side_out,
			            GError        **error)
{
  int pipe_fds[2];
retry:
  if (pipe (pipe_fds) < 0)
    {
      if (eva_errno_is_ignorable (errno))
        goto retry;
      eva_errno_fd_creation_failed ();
      g_set_error (error, EVA_G_ERROR_DOMAIN,
		   eva_error_code_from_errno (errno),
		   "error allocating pipe: %s", g_strerror (errno));
      return FALSE;
    }
  eva_fd_set_close_on_exec (pipe_fds[0], TRUE);
  eva_fd_set_close_on_exec (pipe_fds[1], TRUE);
  eva_fd_set_nonblocking (pipe_fds[0]);
  eva_fd_set_nonblocking (pipe_fds[1]);
  *read_side_out = eva_stream_fd_new (pipe_fds[0], EVA_STREAM_FD_IS_READABLE | EVA_STREAM_FD_IS_POLLABLE);
  *write_side_out = eva_stream_fd_new (pipe_fds[1], EVA_STREAM_FD_IS_WRITABLE | EVA_STREAM_FD_IS_POLLABLE);
  return TRUE;
}

static gboolean
do_socketpair (int fds[2],
               GError **error)
{
retry:
  if (socketpair (AF_UNIX, SOCK_STREAM, 0, fds) < 0)
    {
      if (eva_errno_is_ignorable (errno))
        goto retry;
      eva_errno_fd_creation_failed ();
      g_set_error (error, EVA_G_ERROR_DOMAIN,
                   eva_error_code_from_errno (errno),
                   "error allocating duplex pipe: %s",
                   g_strerror (errno));
      return FALSE;
    }
  eva_fd_set_close_on_exec (fds[0], TRUE);
  eva_fd_set_close_on_exec (fds[1], TRUE);
  return TRUE;
}

/**
 * eva_stream_fd_duplex_pipe:
 * @side_a_out: place to store a reference to a newly allocated stream-fd.
 * @side_b_out: place to store a reference to a newly allocated stream-fd.
 * @error: optional error return value.
 *
 * Create a pair of file-descriptors that 
 * are connected to eachother, and make EvaStreamFds around them.
 *
 * returns: whether the streams were allocated successfully.
 */
gboolean    eva_stream_fd_duplex_pipe (EvaStream     **side_a_out,
                                       EvaStream     **side_b_out,
			               GError        **error)
{
  int fds[2];
  guint flags = EVA_STREAM_FD_IS_READABLE
              | EVA_STREAM_FD_IS_WRITABLE
              | EVA_STREAM_FD_IS_POLLABLE;
  if (!do_socketpair (fds, error))
    return FALSE;
  eva_fd_set_nonblocking (fds[0]);
  eva_fd_set_nonblocking (fds[1]);
  *side_a_out = eva_stream_fd_new (fds[0], flags);
  *side_b_out = eva_stream_fd_new (fds[1], flags);
  return TRUE;
}

/**
 * eva_stream_fd_duplex_pipe_fd:
 * @side_a_out: place to store a reference to a newly allocated stream-fd.
 * @side_b_fd_out: place to store a new file-descriptor.
 * @error: optional error return value.
 *
 * Create a pair of file-descriptors that 
 * are connected to eachother, and make a EvaStreamFd around one of them.
 *
 * returns: whether the file-descriptors were allocated successfully.
 */
gboolean    eva_stream_fd_duplex_pipe_fd (EvaStream     **side_a_out,
                                          int            *side_b_fd_out,
			                  GError        **error)
{
  int fds[2];
  guint flags = EVA_STREAM_FD_IS_READABLE
              | EVA_STREAM_FD_IS_WRITABLE
              | EVA_STREAM_FD_IS_POLLABLE;
  if (!do_socketpair (fds, error))
    return FALSE;
  eva_fd_set_nonblocking (fds[0]);
  *side_a_out = eva_stream_fd_new (fds[0], flags);
  *side_b_fd_out = fds[1];
  return TRUE;
}
