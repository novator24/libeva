#include "evaerror.h"
#include "evaghelpers.h"
#include "evamacros.h"
#include "evastreamtransferrequest.h"

#define DEFAULT_MAX_BUFFERED     4096
#define DEFAULT_MAX_ATOMIC_READ  4096
#define MAX_READ_ON_STACK        8192

static GObjectClass *parent_class = NULL;

static inline void
check_internal_write_block (EvaStreamTransferRequest *request, gboolean block)
{
  if (block && !request->blocking_write_side)
    {
      request->blocking_write_side = 1;
      eva_io_block_write (EVA_IO (request->write_side));
    }
  else if (!block && request->blocking_write_side)
    {
      request->blocking_write_side = 0;
      eva_io_unblock_write (EVA_IO (request->write_side));
    }
}

static inline void
check_internal_read_block (EvaStreamTransferRequest *request, gboolean block)
{
  if (block && !request->blocking_read_side)
    {
      request->blocking_read_side = 1;
      eva_io_block_read (EVA_IO (request->read_side));
    }
  else if (!block && request->blocking_read_side)
    {
      request->blocking_read_side = 0;
      eva_io_unblock_read (EVA_IO (request->read_side));
    }
}

static inline void
check_internal_blocks (EvaStreamTransferRequest *request)
{
  guint size = request->buffer.size;
  check_internal_read_block (request, size > request->max_buffered);
  check_internal_write_block (request, size == 0);
}

static void
handle_error (EvaStreamTransferRequest *self, GError *error)
{
  g_return_if_fail (error);
  g_warning ("EvaStreamTransferRequest: %s", error->message);

  if (eva_request_had_error (self))
    g_free (error);
  else
    {
      g_return_if_fail (eva_request_get_is_running (self));
      g_return_if_fail (!eva_request_get_is_done (self));
      g_return_if_fail (!eva_request_get_is_cancelled (self));

      eva_request_set_error (self, error);
      eva_request_done (self);
      eva_io_read_shutdown (self->read_side, NULL);
      eva_io_write_shutdown (self->write_side, NULL);
    }
}

static gboolean
handle_input_is_readable (EvaIO *io, gpointer user_data)
{
  EvaStreamTransferRequest *self = EVA_STREAM_TRANSFER_REQUEST (user_data);
  EvaStream *read_side = self->read_side;
  EvaStream *write_side = self->write_side;
  guint atomic_read_size = self->atomic_read_size;
  gboolean read_on_stack = (atomic_read_size > MAX_READ_ON_STACK);
  GError *error = NULL;
  char *buf;
  guint num_read;

  g_return_val_if_fail (read_side == EVA_STREAM (io), FALSE);
  g_return_val_if_fail (write_side, FALSE);

  if (read_on_stack)
    buf = g_alloca (atomic_read_size);
  else
    buf = g_malloc (atomic_read_size);

  num_read = eva_stream_read (read_side, buf, atomic_read_size, &error);
  if (error)
    {
      handle_error (self, error);
      if (!read_on_stack)
        g_free (buf);
      return FALSE;
    }
  if (num_read == 0)
    {
      if (!read_on_stack)
        g_free (buf);
      /* Wait for shutdown notification, in case there's an error. */
      return TRUE;
    }

  if (read_on_stack)
    {
      guint num_written = 0;
      if (self->buffer.size == 0)
	{
	  num_written = eva_stream_write (write_side, buf, num_read, &error);
	  if (error)
	    {
	      handle_error (self, error);
	      return FALSE;
	    }
	}
      if (num_written < num_read)
	eva_buffer_append (&self->buffer,
			   buf + num_written,
			   num_read - num_written);
    }
  else
    eva_buffer_append_foreign (&self->buffer, buf, num_read, g_free, buf);

  check_internal_blocks (self);
  return TRUE;
}

static gboolean
handle_input_shutdown_read (EvaIO *io, gpointer user_data)
{
  EvaStreamTransferRequest *self = EVA_STREAM_TRANSFER_REQUEST (user_data);
  EvaStream *write_side = self->write_side;

  g_return_val_if_fail (self->read_side == EVA_STREAM (io), FALSE);

  /* If the write_side has been shut down already, we don't need to do
   * anything.
   */
  if (write_side && eva_stream_get_is_writable (write_side))
    {
      if (io->error)
	{
	  /* If the stream had an error, it's an error. */
	  handle_error (self, g_error_copy (io->error));
	}
      else if (self->buffer.size == 0)
	{
	  /* Otherwise, if there's nothing buffered, try to close write_side. */
	  GError *error = NULL;
	  if (!eva_io_write_shutdown (EVA_IO (write_side), &error))
	    handle_error (self, error);
	}
    }
  return FALSE;
}

static void
handle_input_is_readable_destroy (gpointer user_data)
{
  EvaStreamTransferRequest *self = EVA_STREAM_TRANSFER_REQUEST (user_data);
  EvaStream *read_side = self->read_side;

  g_return_if_fail (read_side);
  g_return_if_fail (!eva_stream_get_is_readable (read_side));
  g_object_unref (read_side);
  self->read_side = NULL;
  g_object_unref (self);
}

static gboolean
handle_output_is_writable (EvaIO *io, gpointer user_data)
{
  EvaStreamTransferRequest *self = EVA_STREAM_TRANSFER_REQUEST (user_data);
  EvaStream *write_side = self->write_side;
  GError *error = NULL;

  g_return_val_if_fail (write_side == EVA_STREAM (io), FALSE);

  if (self->buffer.size > 0)
    {
      eva_stream_write_buffer (write_side, &self->buffer, &error);
      if (error)
	{
	  handle_error (self, error);
	  return FALSE;
	}
    }
  if ((self->read_side == NULL ||
	!eva_stream_get_is_readable (self->read_side)) &&
      self->buffer.size == 0)
    {
      if (!eva_io_write_shutdown (EVA_IO (write_side), &error))
	{
	  handle_error (self, error);
	  return FALSE;
	}
      else
	{
	  /* Wait for shutdown notification. */
	  return TRUE;
	}
    }
  check_internal_blocks (self);
  return TRUE;
}

static gboolean
handle_output_shutdown_write (EvaIO *io, gpointer user_data)
{
  EvaStreamTransferRequest *self = EVA_STREAM_TRANSFER_REQUEST (user_data);
  EvaStream *read_side = self->read_side;

  g_return_val_if_fail (self->write_side == EVA_STREAM (io), FALSE);

  if (read_side && eva_stream_get_is_readable (read_side))
    {
      /* If the read side is still readable, it means the write side
       * shut down before we could write everything out, so we don't
       * care about the write side's error status.
       */
      GError *error;
      error = g_error_new (EVA_G_ERROR_DOMAIN,
			   0, /* TODO */
			   "premature shutdown of write stream");
      handle_error (self, error);
      eva_io_read_shutdown (EVA_IO (self->read_side), NULL);
    }
  else if (io->error)
    {
      /* Otherwise, if the write side had an error, it's an error. */
      handle_error (self, g_error_copy (io->error));
    }
  else
    {
      /* Otherwise, everything worked. */
      eva_request_done (self);
    }
/* TODO: why would you ever want to return true from the shutdown handler? */
  return FALSE;
}

static void
handle_output_is_writable_destroy (gpointer user_data)
{
  EvaStreamTransferRequest *self = EVA_STREAM_TRANSFER_REQUEST (user_data);
  EvaStream *write_side = self->write_side;

  g_return_if_fail (write_side);
  g_return_if_fail (!eva_io_get_is_writable (write_side));
  self->write_side = NULL;
  g_object_unref (write_side);
  g_object_unref (self);
}

/*
 * EvaRequest methods.
 */

static void
eva_stream_transfer_request_start (EvaRequest *request)
{
  EvaStreamTransferRequest *self = EVA_STREAM_TRANSFER_REQUEST (request);
  EvaStream *read_side = self->read_side;
  EvaStream *write_side = self->write_side;

  g_return_if_fail (read_side);
  g_return_if_fail (eva_stream_get_is_readable (read_side));
  g_return_if_fail (!eva_io_has_read_hook (read_side));
  g_return_if_fail (write_side);
  g_return_if_fail (eva_stream_get_is_writable (write_side));
  g_return_if_fail (!eva_io_has_write_hook (write_side));

  g_object_ref (self);
  g_object_ref (self);
  eva_request_mark_is_running (self);

  eva_io_trap_readable (EVA_IO (read_side),
			handle_input_is_readable,
			handle_input_shutdown_read,
			self,
			handle_input_is_readable_destroy);
  eva_io_trap_writable (EVA_IO (write_side),
			handle_output_is_writable,
			handle_output_shutdown_write,
			self,
			handle_output_is_writable_destroy);
}

void
eva_stream_transfer_request_cancelled (EvaRequest *request)
{
  EvaStreamTransferRequest *self = EVA_STREAM_TRANSFER_REQUEST (request);

  if (eva_request_get_is_running (self))
    {
      EvaStream *read_side = self->read_side;
      EvaStream *write_side = self->write_side;

/* XXX: should a cancellation shut down the streams, or just untrap them?
 * (Clearly not, if there's an "only transfer n bytes" mode...)
 */
      g_return_if_fail (read_side);
      g_return_if_fail (write_side);
      eva_io_read_shutdown (read_side, NULL);
      eva_io_write_shutdown (write_side, NULL);
    }
  eva_request_mark_is_cancelled (self);
}

/*
 * GObject methods.
 */

static void
eva_stream_transfer_request_finalize (GObject *object)
{
  EvaStreamTransferRequest *self = EVA_STREAM_TRANSFER_REQUEST (object);

  if (self->read_side)
    g_object_unref (self->read_side);
  if (self->write_side)
    g_object_unref (self->write_side);
  eva_buffer_destruct (&self->buffer);

  (*parent_class->finalize) (object);
}

static void
eva_stream_transfer_request_init (EvaStreamTransferRequest *request)
{
  request->max_buffered = DEFAULT_MAX_BUFFERED;
  request->atomic_read_size = DEFAULT_MAX_ATOMIC_READ;
  eva_buffer_construct (&request->buffer);
}

static void
eva_stream_transfer_request_class_init (EvaRequestClass *request_class)
{
  parent_class = g_type_class_peek_parent (request_class);
  G_OBJECT_CLASS (request_class)->finalize =
    eva_stream_transfer_request_finalize;
  request_class->start = eva_stream_transfer_request_start;
  request_class->cancelled = eva_stream_transfer_request_cancelled;
}

GType
eva_stream_transfer_request_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0))
    {
      static const GTypeInfo type_info =
	{
	  sizeof(EvaStreamTransferRequestClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) eva_stream_transfer_request_class_init,
	  NULL,		/* class_finalize */
	  NULL,		/* class_data */
	  sizeof (EvaStreamTransferRequest),
	  16,		/* n_preallocs */
	  (GInstanceInitFunc) eva_stream_transfer_request_init,
	  NULL		/* value_table */
	};
      type = g_type_register_static (EVA_TYPE_REQUEST,
				     "EvaStreamTransferRequest",
				     &type_info,
				     0);
    }
  return type;
}

/*
 *
 * Public interface.
 *
 */

void
eva_stream_transfer_request_set_max_buffered
	(EvaStreamTransferRequest *request, guint max_buffered)
{
  request->max_buffered = max_buffered;
  check_internal_blocks (request);
}

guint
eva_stream_transfer_request_get_max_buffered
	(EvaStreamTransferRequest *request)
{
  return request->max_buffered;
}

void
eva_stream_transfer_request_set_atomic_read_size
	(EvaStreamTransferRequest *request, guint atomic_read_size)
{
  request->atomic_read_size = atomic_read_size;
}

guint
eva_stream_transfer_request_get_atomic_read_size
	(EvaStreamTransferRequest *request)
{
  return request->atomic_read_size;
}

EvaStreamTransferRequest *
eva_stream_transfer_request_new (EvaStream *input_stream,
				 EvaStream *output_stream)
{
  EvaStreamTransferRequest *request;

  g_return_val_if_fail (input_stream, NULL);
  g_return_val_if_fail (output_stream, NULL);
  g_return_val_if_fail (eva_stream_get_is_readable (input_stream), NULL);
  g_return_val_if_fail (eva_stream_get_is_writable (output_stream), NULL);
  g_return_val_if_fail (!eva_io_has_read_hook (input_stream), NULL);
  g_return_val_if_fail (!eva_io_has_write_hook (output_stream), NULL);

  request = g_object_new (EVA_TYPE_STREAM_TRANSFER_REQUEST, NULL);
  request->read_side = input_stream;
  g_object_ref (input_stream);
  request->write_side = output_stream;
  g_object_ref (output_stream);
  return request;
}
