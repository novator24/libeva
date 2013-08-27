#include "evabufferstream.h"

static GObjectClass *parent_class = NULL;


enum
{
  /* WARNING: this flag is hard-coded
     in the macros eva_buffer_stream_*_strict_max_write(). */
  STRICT_MAX_WRITE = (1<<0),

  /* Whether to shutdown the hook when the buffer empties. */
  DEFERRED_WRITE_SHUTDOWN = (1<<1)
};

enum
{
  /* Whether to shutdown the hook when the buffer empties. */
  DEFERRED_READ_SHUTDOWN = (1<<0)
};
void eva_buffer_stream_mark_deferred_write_shutdown (GskBufferStream *stream);

#define eva_buffer_stream_has_deferred_write_shutdown(stream)	\
  EVA_HOOK_TEST_USER_FLAG (eva_buffer_stream_write_hook(stream), DEFERRED_WRITE_SHUTDOWN)
#define eva_buffer_stream_mark_deferred_write_shutdown(stream)	\
  EVA_HOOK_MARK_USER_FLAG (eva_buffer_stream_write_hook(stream), DEFERRED_WRITE_SHUTDOWN)
#define eva_buffer_stream_clear_deferred_write_shutdown(stream)	\
  EVA_HOOK_CLEAR_USER_FLAG (eva_buffer_stream_write_hook(stream), DEFERRED_WRITE_SHUTDOWN)

#define eva_buffer_stream_has_deferred_read_shutdown(stream)	\
  EVA_HOOK_TEST_USER_FLAG (eva_buffer_stream_read_hook(stream), DEFERRED_READ_SHUTDOWN)
#define eva_buffer_stream_mark_deferred_read_shutdown(stream)	\
  EVA_HOOK_MARK_USER_FLAG (eva_buffer_stream_read_hook(stream), DEFERRED_READ_SHUTDOWN)
#define eva_buffer_stream_clear_deferred_read_shutdown(stream)	\
  EVA_HOOK_CLEAR_USER_FLAG (eva_buffer_stream_read_hook(stream), DEFERRED_READ_SHUTDOWN)

/**
 * eva_buffer_stream_read_shutdown:
 * @stream: the stream to gracefully shut-down.
 *
 * Shutdown the read-end of the buffer-stream,
 * waiting for the buffer to be drained first.
 */
void eva_buffer_stream_read_shutdown (GskBufferStream *stream)
{
  if (stream->read_buffer.size == 0)
    eva_io_notify_read_shutdown (EVA_IO (stream));
  else
    eva_buffer_stream_mark_deferred_read_shutdown (stream);
}

/* --- GskStream methods --- */
static guint
eva_buffer_stream_raw_read (GskStream     *stream,
                            gpointer       data,
                            guint          length,
                            GError       **error)
{
  GskBufferStream *bs = EVA_BUFFER_STREAM (stream);
  GskBuffer *buffer = &bs->read_buffer;
  guint rv = eva_buffer_read (buffer, data, length);
  if (rv > 0)
    eva_buffer_stream_read_buffer_changed (bs);
  return rv;
}

static guint
eva_buffer_stream_raw_write (GskStream     *stream,
                             gconstpointer  data,
                             guint          length,
                             GError       **error)
{
  GskBufferStream *bs = EVA_BUFFER_STREAM (stream);
  GskBuffer *buffer = &bs->write_buffer;
  if (eva_buffer_stream_has_strict_max_write (bs))
    {
      if (buffer->size >= bs->max_write_buffer)
	return 0;
      if (buffer->size + length > bs->max_write_buffer)
	length = bs->max_write_buffer - buffer->size;
    }
  eva_buffer_append (buffer, data, length);
  if (length > 0)
    eva_buffer_stream_write_buffer_changed (bs);
  return length;
}

static guint
eva_buffer_stream_raw_read_buffer (GskStream     *stream,
                                   GskBuffer     *buffer,
                                   GError       **error)
{
  GskBufferStream *bs = EVA_BUFFER_STREAM (stream);
  GskBuffer *read_buffer = &bs->read_buffer;
  guint rv = eva_buffer_drain (buffer, read_buffer);
  if (rv > 0)
    eva_buffer_stream_read_buffer_changed (bs);
  return rv;
}

static guint
eva_buffer_stream_raw_write_buffer (GskStream    *stream,
                                    GskBuffer     *buffer,
                                    GError       **error)
{
  GskBufferStream *bs = EVA_BUFFER_STREAM (stream);
  GskBuffer *write_buffer = &bs->write_buffer;
  guint length = buffer->size;
  guint rv;
  if (eva_buffer_stream_has_strict_max_write (bs))
    {
      if (buffer->size >= bs->max_write_buffer)
	return 0;
      if (buffer->size + length > bs->max_write_buffer)
	length = bs->max_write_buffer - buffer->size;
      rv = eva_buffer_transfer (write_buffer, buffer, length);
    }
  else
    {
      rv = eva_buffer_drain (write_buffer, buffer);
    }
  if (rv > 0)
    eva_buffer_stream_write_buffer_changed (bs);
  return rv;
}

/* --- GskIO methods --- */
/**
 * eva_buffer_stream_read_buffer_changed:
 * @stream: stream whose read buffer has been modified.
 *
 * Called to notify the buffer stream that its
 * read-size has been changed, usually because
 * an implementor has appended data into it
 * for the attached stream to read.
 */
void eva_buffer_stream_read_buffer_changed  (GskBufferStream *stream)
{
  if (stream->read_buffer.size == 0)
    {
      if (eva_buffer_stream_has_deferred_read_shutdown (stream))
	eva_io_notify_read_shutdown (stream);
      else
	eva_io_clear_idle_notify_read (stream);
      eva_hook_set_idle_notify (eva_buffer_stream_read_hook (stream),
                                eva_io_is_polling_for_read (stream));
    }
  else if (eva_io_get_is_readable (stream))
    {
      eva_io_mark_idle_notify_read (stream);
    }
}

/**
 * eva_buffer_stream_write_buffer_changed:
 * @stream: stream whose write buffer has been modified.
 *
 * Called to notify the buffer stream that its
 * write-buffer has been changed, usually because
 * an implementor has read data from it.
 */
void
eva_buffer_stream_write_buffer_changed (GskBufferStream *stream)
{
  if (stream->write_buffer.size < stream->max_write_buffer)
    eva_io_mark_idle_notify_write (stream);
  else
    eva_io_clear_idle_notify_write (stream);

  if (stream->write_buffer.size > 0)
    eva_hook_mark_idle_notify (eva_buffer_stream_write_hook (stream));
  else
    {
      eva_hook_clear_idle_notify (eva_buffer_stream_write_hook (stream));
      if (eva_buffer_stream_has_deferred_write_shutdown (stream))
	{
	  eva_buffer_stream_clear_deferred_write_shutdown (stream);
	  eva_hook_notify_shutdown (eva_buffer_stream_write_hook (stream));
	}
    }
}

/**
 * eva_buffer_stream_changed:
 * @stream: the stream whose internals have been modified.
 *
 * Do all updates needed to compensate for
 * any user changes to: read_buffer, write_buffer,
 * max_write_buffer.
 */
void
eva_buffer_stream_changed              (GskBufferStream *stream)
{
  eva_buffer_stream_read_buffer_changed (stream);
  eva_buffer_stream_write_buffer_changed (stream);
}

static void
eva_buffer_stream_set_poll_read (GskIO      *io,
                                 gboolean    do_poll)
{
  GskBufferStream *bs = EVA_BUFFER_STREAM (io);
  if (bs->read_buffer.size == 0)
    {
      eva_hook_set_idle_notify (eva_buffer_stream_read_hook (bs), do_poll);
    }
  else if (eva_io_get_is_readable (bs))
    {
      g_return_if_fail (eva_io_get_idle_notify_read (bs));
    }
}

static void
eva_buffer_stream_set_poll_write (GskIO      *io,
                                  gboolean    do_poll)
{
  /* Nothing to do.

     All the work is done by idle-notify hooks
     in eva_buffer_stream_write_buffer_changed() */
}

static gboolean
eva_buffer_stream_shutdown_read (GskIO      *io,
                                 GError    **error)
{
  eva_hook_notify_shutdown (eva_buffer_stream_read_hook (EVA_BUFFER_STREAM (io)));
  return TRUE;
}

static gboolean
eva_buffer_stream_shutdown_write (GskIO      *io,
                                  GError    **error)
{
  GskBufferStream *bs = EVA_BUFFER_STREAM (io);
  if (bs->write_buffer.size == 0)
    eva_hook_notify_shutdown (eva_buffer_stream_write_hook (bs));
  else
    {
      eva_buffer_stream_mark_deferred_write_shutdown (bs);

      /* postpone shutdown until it happens for real. */
      return FALSE;
    }

  return TRUE;
}

/* --- GObject methods --- */
static void
eva_buffer_stream_finalize (GObject        *object)
{
  GskBufferStream *bs = EVA_BUFFER_STREAM (object);
  eva_buffer_destruct (&bs->read_buffer);
  eva_buffer_destruct (&bs->write_buffer);
  eva_hook_destruct (&bs->buffered_read_hook);
  eva_hook_destruct (&bs->buffered_write_hook);
  (*parent_class->finalize) (object);
}

/* --- functions --- */
static void
eva_buffer_stream_init (GskBufferStream *buffer_stream)
{
  EVA_HOOK_INIT (buffer_stream,
		 GskBufferStream,
		 buffered_read_hook,
		 EVA_HOOK_IS_AVAILABLE,
		 buffered_read_set_poll, buffered_read_shutdown);
  EVA_HOOK_INIT (buffer_stream,
		 GskBufferStream,
		 buffered_write_hook,
		 EVA_HOOK_IS_AVAILABLE,
		 buffered_write_set_poll, buffered_write_shutdown);

  buffer_stream->max_write_buffer = 4096;
  eva_stream_mark_is_writable (buffer_stream);
  eva_stream_mark_is_readable (buffer_stream);
  eva_buffer_stream_changed (buffer_stream);
}
static void
eva_buffer_stream_class_init (GskBufferStreamClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GskIOClass *io_class = EVA_IO_CLASS (class);
  GskStreamClass *stream_class = EVA_STREAM_CLASS (class);
  parent_class = g_type_class_peek_parent (class);
  object_class->finalize = eva_buffer_stream_finalize;
  io_class->set_poll_read = eva_buffer_stream_set_poll_read;
  io_class->set_poll_write = eva_buffer_stream_set_poll_write;
  io_class->shutdown_read = eva_buffer_stream_shutdown_read;
  io_class->shutdown_write = eva_buffer_stream_shutdown_write;
  stream_class->raw_read = eva_buffer_stream_raw_read;
  stream_class->raw_write = eva_buffer_stream_raw_write;
  stream_class->raw_read_buffer = eva_buffer_stream_raw_read_buffer;
  stream_class->raw_write_buffer = eva_buffer_stream_raw_write_buffer;
  EVA_HOOK_CLASS_INIT (object_class, "buffered-read-hook", GskBufferStream, buffered_read_hook);
  EVA_HOOK_CLASS_INIT (object_class, "buffered-write-hook", GskBufferStream, buffered_write_hook);
}

GType eva_buffer_stream_get_type()
{
  static GType buffer_stream_type = 0;
  if (!buffer_stream_type)
    {
      static const GTypeInfo buffer_stream_info =
      {
	sizeof(GskBufferStreamClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_buffer_stream_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GskBufferStream),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_buffer_stream_init,
	NULL		/* value_table */
      };
      buffer_stream_type = g_type_register_static (EVA_TYPE_STREAM,
                                                  "GskBufferStream",
						  &buffer_stream_info, 0);
    }
  return buffer_stream_type;
}

/**
 * eva_buffer_stream_new:
 *
 * Create a new #GskBufferStream.
 *
 * returns: the newly allocated GskBufferStream.
 */
GskBufferStream *
eva_buffer_stream_new (void)
{
  return g_object_new (EVA_TYPE_BUFFER_STREAM, NULL);
}
