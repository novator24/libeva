#include <string.h>
#include "evamemory.h"

/* === EvaMemoryBufferSource === */
typedef struct _EvaMemoryBufferSource EvaMemoryBufferSource;
typedef struct _EvaMemoryBufferSourceClass EvaMemoryBufferSourceClass;
static GType eva_memory_buffer_source_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_MEMORY_BUFFER_SOURCE			(eva_memory_buffer_source_get_type ())
#define EVA_MEMORY_BUFFER_SOURCE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_MEMORY_BUFFER_SOURCE, EvaMemoryBufferSource))
#define EVA_MEMORY_BUFFER_SOURCE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_MEMORY_BUFFER_SOURCE, EvaMemoryBufferSourceClass))
#define EVA_MEMORY_BUFFER_SOURCE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_MEMORY_BUFFER_SOURCE, EvaMemoryBufferSourceClass))
#define EVA_IS_MEMORY_BUFFER_SOURCE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_MEMORY_BUFFER_SOURCE))
#define EVA_IS_MEMORY_BUFFER_SOURCE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_MEMORY_BUFFER_SOURCE))

struct _EvaMemoryBufferSourceClass 
{
  EvaStreamClass stream_class;
};
struct _EvaMemoryBufferSource 
{
  EvaStream      stream;
  EvaBuffer      buffer;
};
static GObjectClass *global_stream_class = NULL;

static guint
eva_memory_buffer_source_raw_read  (EvaStream     *stream,
			 	    gpointer       data,
			 	    guint          length,
			 	    GError       **error)
{
  EvaMemoryBufferSource *source = EVA_MEMORY_BUFFER_SOURCE (stream);
  guint rv = eva_buffer_read (&source->buffer, data, length);
  if (rv == 0 && source->buffer.size == 0)
    eva_io_notify_read_shutdown (stream);
  return rv;
}

static guint
eva_memory_buffer_source_raw_read_buffer (EvaStream     *stream,
					  EvaBuffer     *buffer,
					  GError       **error)
{
  EvaMemoryBufferSource *source = EVA_MEMORY_BUFFER_SOURCE (stream);
  guint rv = eva_buffer_drain (buffer, &source->buffer);
  if (rv == 0)
    eva_io_notify_read_shutdown (stream);
  return rv;
}

static void
eva_memory_buffer_source_finalize(GObject *object)
{
  EvaMemoryBufferSource *source = EVA_MEMORY_BUFFER_SOURCE (object);
  eva_buffer_destruct (&source->buffer);
  (*global_stream_class->finalize) (object);
}

static void
eva_memory_buffer_source_init (EvaMemoryBufferSource *memory_buffer_source)
{
  eva_stream_mark_is_readable (memory_buffer_source);
  eva_stream_mark_never_blocks_read (memory_buffer_source);
}

static void
eva_memory_buffer_source_class_init (EvaMemoryBufferSourceClass *class)
{
  EvaStreamClass *stream_class = EVA_STREAM_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  global_stream_class = g_type_class_peek_parent (class);
  stream_class->raw_read = eva_memory_buffer_source_raw_read;
  stream_class->raw_read_buffer = eva_memory_buffer_source_raw_read_buffer;
  object_class->finalize = eva_memory_buffer_source_finalize;
}

static GType eva_memory_buffer_source_get_type()
{
  static GType memory_buffer_source_type = 0;
  if (!memory_buffer_source_type)
    {
      static const GTypeInfo memory_buffer_source_info =
      {
	sizeof(EvaMemoryBufferSourceClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_memory_buffer_source_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EvaMemoryBufferSource),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_memory_buffer_source_init,
	NULL		/* value_table */
      };
      memory_buffer_source_type = g_type_register_static (EVA_TYPE_STREAM,
                                                  "EvaMemoryBufferSource",
						  &memory_buffer_source_info, 0);
    }
  return memory_buffer_source_type;
}

/**
 * eva_memory_buffer_source_new:
 * @buffer: buffer whose contents should be readable from the new stream.
 * It will be immediately (before the function returns) drained of all
 * data, and will not be used any more by the stream.
 *
 * Create a read-only #EvaStream that will drain the 
 * data from @buffer and may it available for reading on the stream.
 *
 * returns: the new read-only stream.
 */
EvaStream *
eva_memory_buffer_source_new (EvaBuffer              *buffer)
{
  EvaMemoryBufferSource *source;
  g_return_val_if_fail (buffer != NULL, NULL);
  source = g_object_new (EVA_TYPE_MEMORY_BUFFER_SOURCE, NULL);
  eva_buffer_drain (&source->buffer, buffer);
  return EVA_STREAM (source);
}

/* === EvaMemorySlabSource === */
typedef struct _EvaMemorySlabSource EvaMemorySlabSource;
typedef struct _EvaMemorySlabSourceClass EvaMemorySlabSourceClass;
GType eva_memory_slab_source_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_MEMORY_SLAB_SOURCE			(eva_memory_slab_source_get_type ())
#define EVA_MEMORY_SLAB_SOURCE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_MEMORY_SLAB_SOURCE, EvaMemorySlabSource))
#define EVA_MEMORY_SLAB_SOURCE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_MEMORY_SLAB_SOURCE, EvaMemorySlabSourceClass))
#define EVA_MEMORY_SLAB_SOURCE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_MEMORY_SLAB_SOURCE, EvaMemorySlabSourceClass))
#define EVA_IS_MEMORY_SLAB_SOURCE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_MEMORY_SLAB_SOURCE))
#define EVA_IS_MEMORY_SLAB_SOURCE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_MEMORY_SLAB_SOURCE))

struct _EvaMemorySlabSourceClass 
{
  EvaStreamClass stream_class;
};
struct _EvaMemorySlabSource 
{
  EvaStream      	  stream;
  gconstpointer           data;
  guint                   data_len;
  GDestroyNotify          destroy;
  gpointer                destroy_data;
};

static guint
eva_memory_slab_source_raw_read  (EvaStream     *stream,
				  gpointer       data,
				  guint          length,
				  GError       **error)
{
  EvaMemorySlabSource *source = EVA_MEMORY_SLAB_SOURCE (stream);
  guint rv = MIN (source->data_len, length);
  if (rv != 0)
    {
      memcpy (data, source->data, rv);
      source->data = ((const char *)source->data) + rv;
      source->data_len -= rv;
    }

  if (source->data_len == 0)
    eva_io_notify_read_shutdown (stream);
  return rv;
}

static guint
eva_memory_slab_source_raw_read_buffer (EvaStream     *stream,
					EvaBuffer     *buffer,
					GError       **error)
{
  EvaMemorySlabSource *source = EVA_MEMORY_SLAB_SOURCE (stream);
  guint rv = source->data_len;
  if (rv != 0)
    {
      eva_buffer_append_foreign (buffer, source->data, source->data_len,
				 source->destroy, source->destroy_data);
      source->data_len = 0;
      source->destroy = NULL;
    }
  eva_io_notify_read_shutdown (stream);
  return rv;
}

static void
eva_memory_slab_source_finalize (GObject *object)
{
  EvaMemorySlabSource *source = EVA_MEMORY_SLAB_SOURCE (object);
  if (source->destroy)
    source->destroy (source->destroy_data);
  global_stream_class->finalize (object);
}

static void
eva_memory_slab_source_init (EvaMemorySlabSource *memory_slab_source)
{
  eva_stream_mark_is_readable (memory_slab_source);
  eva_stream_mark_never_blocks_read (memory_slab_source);
}

static void
eva_memory_slab_source_class_init (EvaMemorySlabSourceClass *class)
{
  EvaStreamClass *stream_class = EVA_STREAM_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  global_stream_class = g_type_class_peek_parent (class);
  stream_class->raw_read = eva_memory_slab_source_raw_read;
  stream_class->raw_read_buffer = eva_memory_slab_source_raw_read_buffer;
  object_class->finalize = eva_memory_slab_source_finalize;
}

GType eva_memory_slab_source_get_type()
{
  static GType memory_slab_source_type = 0;
  if (!memory_slab_source_type)
    {
      static const GTypeInfo memory_slab_source_info =
      {
	sizeof(EvaMemorySlabSourceClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_memory_slab_source_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EvaMemorySlabSource),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_memory_slab_source_init,
	NULL		/* value_table */
      };
      memory_slab_source_type = g_type_register_static (EVA_TYPE_STREAM,
                                                  "EvaMemorySlabSource",
						  &memory_slab_source_info, 0);
    }
  return memory_slab_source_type;
}

/**
 * eva_memory_slab_source_new:
 * @data: binary data which will be readable from the returned stream.
 * @data_len: length of the returned stream.
 * @destroy: method called by the stream once @data is completely used
 * (ie, the user has read all the data or shutdown the stream)
 * @destroy_data: data passed to @destroy.
 *
 * Create a read-only stream which has certain, given data available for reading.
 *
 * For efficiency, this code does not copy the data, but rather calls
 * a user-supplied destroy method when we are done.  
 *
 * One possibility if you need the data copied, is to call
 * g_memdup on data, then pass in g_free for @destroy and the copy of the data
 * as @destroy_data.
 *
 * returns: the new read-only stream.
 */
EvaStream *
eva_memory_slab_source_new   (gconstpointer           data,
			      guint                   data_len,
			      GDestroyNotify          destroy,
			      gpointer                destroy_data)
{
  EvaMemorySlabSource *slab_source;
  slab_source = g_object_new (EVA_TYPE_MEMORY_SLAB_SOURCE, NULL);
  slab_source->data = data;
  slab_source->data_len = data_len;
  slab_source->destroy = destroy;
  slab_source->destroy_data = destroy_data;
  return EVA_STREAM (slab_source);
}

/**
 * eva_memory_source_new_printf:
 * @format: a printf(3) format string.
 * @...: arguments, as used by printf(3).
 *
 * Create a read-only stream which has the result of doing the
 * sprintf available for reading.
 *
 * returns: the new read-only stream.
 */
EvaStream *
eva_memory_source_new_printf (const char             *format,
			      ...)
{
  char *str;
  va_list args;
  va_start (args, format);
  str = g_strdup_vprintf (format, args);
  va_end (args);
  return eva_memory_slab_source_new (str, strlen (str), g_free, str);
}

/**
 * eva_memory_source_new_vprintf:
 * @format: a printf(3) format string.
 * @args: arguments, as used by vprintf(3).
 *
 * Create a read-only stream which has the result of doing the
 * vsprintf available for reading.  This is useful for
 * chaining from other printf-like functions.
 *
 * returns: the new read-only stream.
 */
EvaStream *
eva_memory_source_new_vprintf (const char             *format,
			       va_list                 args)
{
  char *str = g_strdup_vprintf (format, args);
  return eva_memory_slab_source_new (str, strlen (str), g_free, str);
}

/**
 * eva_memory_source_static_string:
 * @str: the static string which is the content of the stream.
 *
 * Create a read-only stream which has @str available
 * for reading.
 *
 * Note that the NUL will not be read from the stream.
 *
 * returns: the new read-only stream.
 */
EvaStream *
eva_memory_source_static_string (const char *str)
{
  return eva_memory_slab_source_new (str, strlen (str), NULL, NULL);
}

/**
 * eva_memory_source_static_string_n:
 * @str: the static string or binary-data which is the content of the stream.
 * @length: length of the stream in bytes.
 *
 * Create a read-only stream which has @length bytes starting at @str available
 * for reading.
 *
 * returns: the new read-only stream.
 */
EvaStream *
eva_memory_source_static_string_n (const char *str,
				   guint       length)
{
  return eva_memory_slab_source_new (str, length, NULL, NULL);
}

/* --- streams which can be written to --- */

/* === EvaMemorySink === */

/* Insert header here. */
typedef struct _EvaMemorySink EvaMemorySink;
typedef struct _EvaMemorySinkClass EvaMemorySinkClass;
GType eva_memory_sink_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_MEMORY_SINK			(eva_memory_sink_get_type ())
#define EVA_MEMORY_SINK(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_MEMORY_SINK, EvaMemorySink))
#define EVA_MEMORY_SINK_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_MEMORY_SINK, EvaMemorySinkClass))
#define EVA_MEMORY_SINK_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_MEMORY_SINK, EvaMemorySinkClass))
#define EVA_IS_MEMORY_SINK(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_MEMORY_SINK))
#define EVA_IS_MEMORY_SINK_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_MEMORY_SINK))

struct _EvaMemorySinkClass 
{
  EvaStreamClass stream_class;
};
struct _EvaMemorySink 
{
  EvaStream      stream;

  /* underlying data store */
  EvaBuffer      buffer;

  /* from the user; callback and destroy are set to NULL when they are
     run, since they should only be run once. */
  EvaMemoryBufferCallback callback;
  gpointer                data;
  GDestroyNotify          destroy;
};

static guint
eva_memory_sink_raw_write       (EvaStream     *stream,
			 	 gconstpointer  data,
			 	 guint          length,
			 	 GError       **error)
{
  eva_buffer_append (&EVA_MEMORY_SINK (stream)->buffer, data, length);
  return length;
}

static guint
eva_memory_sink_raw_write_buffer(EvaStream    *stream,
				 EvaBuffer     *buffer,
				 GError       **error)
{
  return eva_buffer_drain (&EVA_MEMORY_SINK (stream)->buffer, buffer);
}

static gboolean
eva_memory_sink_shutdown_write(EvaIO      *io,
			       GError    **error)
{
  EvaMemorySink *sink = EVA_MEMORY_SINK (io);
  if (sink->callback)
    {
      EvaMemoryBufferCallback callback = sink->callback;
      sink->callback = NULL;
      (*callback) (&sink->buffer, sink->data);
    }
  eva_buffer_destruct (&sink->buffer);
  return TRUE;
}

static void
eva_memory_sink_finalize (GObject *object)
{
  EvaMemorySink *sink = EVA_MEMORY_SINK (object);
  eva_buffer_destruct (&sink->buffer);
  if (sink->destroy != NULL)
    (*sink->destroy) (sink->data);
  (*global_stream_class->finalize) (object);
}

static void
eva_memory_sink_init (EvaMemorySink *memory_sink)
{
  eva_io_mark_is_writable (memory_sink);
  eva_stream_mark_never_blocks_write (memory_sink);
  eva_stream_mark_never_partial_writes (memory_sink);
}

static void
eva_memory_sink_class_init (EvaMemorySinkClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  EvaIOClass *io_class = EVA_IO_CLASS (class);
  EvaStreamClass *stream_class = EVA_STREAM_CLASS (class);

  global_stream_class = g_type_class_peek_parent (class);

  object_class->finalize = eva_memory_sink_finalize;
  io_class->shutdown_write = eva_memory_sink_shutdown_write;
  stream_class->raw_write = eva_memory_sink_raw_write;
  stream_class->raw_write_buffer = eva_memory_sink_raw_write_buffer;
}

GType eva_memory_sink_get_type()
{
  static GType memory_sink_type = 0;
  if (!memory_sink_type)
    {
      static const GTypeInfo memory_sink_info =
      {
	sizeof(EvaMemorySinkClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_memory_sink_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EvaMemorySink),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_memory_sink_init,
	NULL		/* value_table */
      };
      memory_sink_type = g_type_register_static (EVA_TYPE_STREAM,
                                                  "EvaMemorySink",
						  &memory_sink_info, 0);
    }
  return memory_sink_type;
}

/**
 * eva_memory_buffer_sink_new:
 * @callback: function to call when the buffer has been filled.
 * @data: user-data to pass to the @callback.
 * @destroy: function to call when we are done with the data.
 *
 * Create a sink for binary data which just fills
 * a binary buffer.  When the stream is done,
 * the @callback will be run with the full buffer.
 *
 * returns: the writable stream whose destination is a #EvaBuffer.
 */
EvaStream *eva_memory_buffer_sink_new   (EvaMemoryBufferCallback callback,
					 gpointer                data,
					 GDestroyNotify          destroy)
{
  EvaMemorySink *sink = g_object_new (EVA_TYPE_MEMORY_SINK, NULL);
  sink->callback = callback;
  sink->data = data;
  sink->destroy = destroy;
  return EVA_STREAM (sink);
}
