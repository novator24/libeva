#include "evasimplefilter.h"
#include "evamacros.h"

static GObjectClass *parent_class = NULL;

#define DEFAULT_MAX_READ_BUFFER_SIZE		8192
#define DEFAULT_MAX_WRITE_BUFFER_SIZE		8192

static inline void
update_idle_notification (EvaSimpleFilter *filter)
{
  if (!eva_io_get_is_writable (filter) && filter->read_buffer.size == 0)
    eva_io_notify_read_shutdown (filter);
  else
    {
      eva_io_set_idle_notify_read (filter, filter->read_buffer.size > 0);
      eva_io_set_idle_notify_write (filter,
				    filter->write_buffer.size < filter->max_write_buffer_size
				    && filter->read_buffer.size < filter->max_read_buffer_size);
    }
}

static gboolean
process_data (EvaSimpleFilter *filter,
	      GError         **error)
{
  EvaSimpleFilterClass *class = EVA_SIMPLE_FILTER_GET_CLASS (filter);
  g_return_val_if_fail (class->process != NULL, FALSE);
  if (!(*class->process) (filter, &filter->read_buffer, &filter->write_buffer, error))
    return FALSE;
  return TRUE;
}

/* --- EvaStream methods --- */
static guint
eva_simple_filter_raw_read (EvaStream     *stream,
                            gpointer       data,
                            guint          length,
                            GError       **error)
{
  EvaSimpleFilter *simple_filter = EVA_SIMPLE_FILTER (stream);
  guint rv = eva_buffer_read (&simple_filter->read_buffer, data, length);

  update_idle_notification (simple_filter);

  return rv;
}

static guint
eva_simple_filter_raw_write (EvaStream     *stream,
                             gconstpointer  data,
                             guint          length,
                             GError       **error)
{
  EvaSimpleFilter *simple_filter = EVA_SIMPLE_FILTER (stream);
  eva_buffer_append (&simple_filter->write_buffer, data, length);
  if (!process_data (simple_filter, error))
    return length;
  update_idle_notification (simple_filter);
  return length;
}

static guint
eva_simple_filter_raw_read_buffer (EvaStream     *stream,
                                   EvaBuffer     *buffer,
                                   GError       **error)
{
  EvaSimpleFilter *simple_filter = EVA_SIMPLE_FILTER (stream);
  guint rv = eva_buffer_drain (buffer, &simple_filter->read_buffer);
  update_idle_notification (simple_filter);
  return rv;
}

static guint
eva_simple_filter_raw_write_buffer (EvaStream    *stream,
                                    EvaBuffer     *buffer,
                                    GError       **error)
{
  EvaSimpleFilter *simple_filter = EVA_SIMPLE_FILTER (stream);
  guint rv = eva_buffer_drain (&simple_filter->write_buffer, buffer);
  if (!process_data (simple_filter, error))
    return rv;
  update_idle_notification (simple_filter);
  return rv;
}

/* --- EvaIO methods --- */
#if 0
static void
eva_simple_filter_set_poll_read (EvaIO      *io,
                                 gboolean    do_poll)
{
  ...
}

static void
eva_simple_filter_set_poll_write (EvaIO      *io,
                                  gboolean    do_poll)
{
  ...
}
#endif

static gboolean
eva_simple_filter_shutdown_read (EvaIO      *io,
                                 GError    **error)
{
  EvaSimpleFilter *simple_filter = EVA_SIMPLE_FILTER (io);
  if (simple_filter->write_buffer.size > 0)
    {
      eva_io_set_error (io, EVA_IO_ERROR_READ,
			EVA_ERROR_LINGERING_DATA,
			_("shutdown_read lost %u bytes"),
			simple_filter->write_buffer.size);
    }
  eva_io_notify_write_shutdown (io);
  return (simple_filter->write_buffer.size == 0
       && simple_filter->read_buffer.size == 0);
}

static gboolean
eva_simple_filter_shutdown_write (EvaIO      *io,
                                  GError    **error)
{
  EvaSimpleFilter *filter = EVA_SIMPLE_FILTER (io);
  EvaSimpleFilterClass *class = EVA_SIMPLE_FILTER_GET_CLASS (io);
  gboolean rv = TRUE;
  if (filter->write_buffer.size > 0)
    if (!(*class->process) (filter, &filter->read_buffer, &filter->write_buffer, error))
      rv = FALSE;
  if (rv && class->flush != NULL)
    if (!(*class->flush) (filter,
			  &filter->read_buffer,
			  &filter->write_buffer,
			  error))
      rv = FALSE;
  update_idle_notification (filter);
  if (filter->read_buffer.size == 0)
    eva_io_notify_read_shutdown (filter);
  return rv;
}

/* --- GObject methods --- */
static void
eva_simple_filter_finalize (GObject        *object)
{
  EvaSimpleFilter *simple_filter = EVA_SIMPLE_FILTER (object);
  eva_buffer_destruct (&simple_filter->read_buffer);
  eva_buffer_destruct (&simple_filter->write_buffer);
  (*parent_class->finalize) (object);
}

/* --- functions --- */
static void
eva_simple_filter_init (EvaSimpleFilter *simple_filter)
{
  EvaStream *stream = EVA_STREAM (simple_filter);
  simple_filter->max_read_buffer_size = DEFAULT_MAX_READ_BUFFER_SIZE;
  simple_filter->max_write_buffer_size = DEFAULT_MAX_WRITE_BUFFER_SIZE;
  eva_stream_mark_is_readable (stream);
  eva_stream_mark_is_writable (stream);
  eva_stream_mark_idle_notify_write (stream);
}
static void
eva_simple_filter_class_init (EvaSimpleFilterClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  EvaIOClass *io_class = EVA_IO_CLASS (class);
  EvaStreamClass *stream_class = EVA_STREAM_CLASS (class);
  parent_class = g_type_class_peek_parent (class);
  object_class->finalize = eva_simple_filter_finalize;
#if 0
  io_class->set_poll_read = eva_simple_filter_set_poll_read;
  io_class->set_poll_write = eva_simple_filter_set_poll_write;
#endif
  io_class->shutdown_read = eva_simple_filter_shutdown_read;
  io_class->shutdown_write = eva_simple_filter_shutdown_write;
  stream_class->raw_read = eva_simple_filter_raw_read;
  stream_class->raw_write = eva_simple_filter_raw_write;
  stream_class->raw_read_buffer = eva_simple_filter_raw_read_buffer;
  stream_class->raw_write_buffer = eva_simple_filter_raw_write_buffer;
}

GType eva_simple_filter_get_type()
{
  static GType simple_filter_type = 0;
  if (!simple_filter_type)
    {
      static const GTypeInfo simple_filter_info =
      {
	sizeof(EvaSimpleFilterClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_simple_filter_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EvaSimpleFilter),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_simple_filter_init,
	NULL		/* value_table */
      };
      simple_filter_type = g_type_register_static (EVA_TYPE_STREAM,
                                                  "EvaSimpleFilter",
						  &simple_filter_info, 0);
    }
  return simple_filter_type;
}
