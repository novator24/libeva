#include "evastreamqueue.h"

static GObjectClass *parent_class = NULL;

#define READ_EMPTY_HOOK(queue)    EVA_STREAM_QUEUE_READ_EMPTY_HOOK(queue)
#define WRITE_EMPTY_HOOK(queue)   EVA_STREAM_QUEUE_WRITE_EMPTY_HOOK(queue)

static gboolean handle_read_stream_notify          (EvaStream *read_substream,
                                                    EvaStreamQueue *);
static gboolean handle_read_stream_shutdown_notify (EvaStream *read_substream,
                                                    EvaStreamQueue *);
static void     handle_read_stream_trap_destroy    (EvaStreamQueue *);
static gboolean handle_write_stream_notify         (EvaStream *write_substream,
                                                    EvaStreamQueue *);
static gboolean handle_write_stream_shutdown_notify(EvaStream *write_substream,
                                                    EvaStreamQueue *);
static void     handle_write_stream_trap_destroy   (EvaStreamQueue *);

static inline void
trap_head_read_stream (EvaStreamQueue *queue)
{
  eva_io_trap_readable (EVA_IO (queue->read_streams->head->data),
                        handle_read_stream_notify,
                        handle_read_stream_shutdown_notify,
                        queue,
                        (GDestroyNotify) handle_read_stream_trap_destroy);
}

static inline void
trap_head_write_stream (EvaStreamQueue *queue)
{
  eva_io_trap_writable (EVA_IO (queue->write_streams->head->data),
                        handle_write_stream_notify,
                        handle_write_stream_shutdown_notify,
                        queue,
                        (GDestroyNotify) handle_write_stream_trap_destroy);
}

static void
dequeue_read_stream (EvaStreamQueue *queue)
{
  EvaStream *s = g_queue_pop_head (queue->read_streams);
  eva_io_untrap_readable (s);
  if (eva_io_is_polling_for_read (queue)
   && queue->read_streams->head != NULL)
    trap_head_read_stream (queue);
  g_object_unref (s);
}

static void
dequeue_write_stream (EvaStreamQueue *queue)
{
  EvaStream *s = g_queue_pop_head (queue->write_streams);
  eva_io_untrap_writable (s);
  if (eva_io_is_polling_for_write (queue)
   && queue->write_streams->head != NULL)
    trap_head_write_stream (queue);
  g_object_unref (s);
}

static inline gboolean
should_be_read_shutdown (EvaStreamQueue *queue)
{
  return queue->no_more_read_streams
      && queue->read_streams->head == NULL;
}

static inline gboolean
should_be_write_shutdown (EvaStreamQueue *queue)
{
  return queue->no_more_write_streams
      && queue->write_streams->head == NULL;
}

static inline void
maybe_read_shutdown_notify (EvaStreamQueue *queue)
{
  if (should_be_read_shutdown (queue))
    eva_io_notify_read_shutdown (queue);
}

static inline void
maybe_write_shutdown_notify (EvaStreamQueue *queue)
{
  if (should_be_write_shutdown (queue))
    eva_io_notify_write_shutdown (queue);
}

/* --- handling child events --- */
static gboolean
handle_read_stream_notify          (EvaStream *read_substream,
                                    EvaStreamQueue *queue)
{
  eva_io_notify_ready_to_read (queue);
  return TRUE;
}

static gboolean
handle_read_stream_shutdown_notify (EvaStream *read_substream,
                                    EvaStreamQueue *queue)
{
  if (!queue->is_reading)
    {
      /* shutdown came from event other than reading */
      dequeue_read_stream (queue);
      maybe_read_shutdown_notify (queue);
    }
  return FALSE;
}

static void    
handle_read_stream_trap_destroy    (EvaStreamQueue *queue)
{
}

static gboolean
handle_write_stream_notify         (EvaStream *write_substream,
                                    EvaStreamQueue *queue)
{
  eva_io_notify_ready_to_write (queue);
  return TRUE;
}
static gboolean
handle_write_stream_shutdown_notify(EvaStream *write_substream,
                                    EvaStreamQueue *queue)
{
  if (!queue->is_writing)
    {
      /* shutdown came from event other than writing */
      dequeue_write_stream (queue);
      maybe_write_shutdown_notify (queue);
    }
  return FALSE;
}
static void    
handle_write_stream_trap_destroy   (EvaStreamQueue *queue)
{
}

/* --- functions --- */
static void
eva_stream_queue_set_poll_read   (EvaIO      *io,
			          gboolean    do_poll)
{
  EvaStreamQueue *queue = EVA_STREAM_QUEUE (io);
  if (queue->read_streams->head == NULL)
    return;
  if (do_poll)
    trap_head_read_stream (queue);
  else
    eva_stream_untrap_readable (queue->read_streams->head->data);
}

static void
eva_stream_queue_set_poll_write  (EvaIO      *io,
			          gboolean    do_poll)
{
  EvaStreamQueue *queue = EVA_STREAM_QUEUE (io);
  if (queue->write_streams->head == NULL)
    return;
  if (do_poll)
    trap_head_write_stream (queue);
  else
    eva_stream_untrap_writable (queue->write_streams->head->data);
}

static GError *
conglomerate_error_list (GSList *errors)
{
  GError *rv;
  if (errors == NULL)
    rv = NULL;
  else if (errors->next == NULL)
    {
      /* special print for 1 error case */
      rv = errors->data;
      g_slist_free (errors);
    }
  else
    {
      /* handle general N error case */
      GString *message = g_string_new ("");
      g_string_append_printf (message, "There were %u errors shutting down queue:\n",
			      g_slist_length (errors));
      for (errors = g_slist_reverse (errors);
	   errors != NULL;
	   errors = g_slist_remove (errors, errors->data))
	{
	  GError *suberror = errors->data;
	  g_string_append_printf (message, "   %s\n", suberror->message);
	  g_error_free (suberror);
	}
      rv = g_error_new (EVA_G_ERROR_DOMAIN, EVA_ERROR_MULTIPLE_PROBLEMS,
		   "%s", message->str);
      g_string_free (message, TRUE);
    }
  return rv;
}

static gboolean
eva_stream_queue_shutdown_read   (EvaIO      *io,
			          GError    **error)
{
  EvaStreamQueue *queue = EVA_STREAM_QUEUE (io);
  GList *at;
  GSList *errors = NULL;
  GError *suberror = NULL;
  EvaStream **streams = g_newa (EvaStream *, queue->read_streams->length);
  guint n = 0;
  guint i;
  for (at = queue->read_streams->head; at != NULL; at = at->next)
    streams[n++] = EVA_STREAM (at->data);

  for (i = 0; i < n; i++)
    if (!eva_io_read_shutdown (streams[i], &suberror))
      {
	errors = g_slist_prepend (errors, suberror);
	suberror = NULL;
      }

  /* clear our list of children */
  g_list_foreach (queue->read_streams->head, (GFunc) g_object_unref, NULL);
  g_queue_free (queue->read_streams);
  queue->read_streams = g_queue_new ();

  if (errors == NULL)
    return TRUE;
  *error = conglomerate_error_list (errors);
  return FALSE;
}

static gboolean
eva_stream_queue_shutdown_write  (EvaIO      *io,
			          GError    **error)
{
  EvaStreamQueue *queue = EVA_STREAM_QUEUE (io);
  GList *at;
  GSList *errors = NULL;
  GError *suberror = NULL;
  EvaStream **streams = g_newa (EvaStream *, queue->write_streams->length);
  guint n = 0;
  guint i;
  for (at = queue->write_streams->head; at != NULL; at = at->next)
    streams[n++] = EVA_STREAM (at->data);
  for (i = 0; i < n; i++)
    if (!eva_io_write_shutdown (streams[i], &suberror))
      {
	errors = g_slist_prepend (errors, suberror);
	suberror = NULL;
      }

  /* clear our list of children */
  g_list_foreach (queue->write_streams->head, (GFunc) g_object_unref, NULL);
  g_queue_free (queue->write_streams);
  queue->write_streams = g_queue_new ();

  if (errors == NULL)
    return TRUE;
  *error = conglomerate_error_list (errors);
  return FALSE;
}

typedef enum
{
  RES_STOP_TRYING,
  RES_DONE_WITH_STREAM,
  RES_TRY_AGAIN
} Res;

static Res   try_one_read  (EvaStreamQueue *queue,
                            gpointer        data,
                            guint           length,
                            guint          *rv_inout,
                            guint           call_count,
                            GError        **error)
{
  EvaStream *substream = EVA_STREAM (queue->read_streams->head->data);
  guint max_len = length - (*rv_inout);
  guint subread = eva_stream_read (substream,
                                   (char *) data + (*rv_inout),
                                   length - (*rv_inout),
                                   error);
  *rv_inout += subread;
  if (call_count > 0             /* only retry if this is the first time through */
   || subread == max_len         /* don't retry if done */
   || *error)                    /* don't retry if an error occurred */
    return RES_STOP_TRYING;
  return eva_io_get_is_readable (substream) ? RES_TRY_AGAIN : RES_DONE_WITH_STREAM;
}

static guint
eva_stream_queue_raw_read   (EvaStream     *stream,
			     gpointer       data,
			     guint          length,
			     GError       **error)
{
  EvaStreamQueue *queue = EVA_STREAM_QUEUE (stream);
  unsigned rv = 0;
  guint count = 0;
  queue->is_reading = 1;
  while (queue->read_streams->head != NULL)
    {
      switch (try_one_read (queue, data, length, &rv, count++, error))
        {
        case RES_STOP_TRYING:
          goto done;
        case RES_DONE_WITH_STREAM:
          dequeue_read_stream (queue);
          break;
        case RES_TRY_AGAIN:
          break;
        }
    }
done:
  queue->is_reading = 0;
  if (should_be_read_shutdown (queue) && rv == 0)
    eva_io_notify_read_shutdown (EVA_IO (stream));
  return rv;
}

static Res
try_one_read_buffer  (EvaStreamQueue *queue,
                      EvaBuffer      *out,
                      guint          *rv_inout,
                      guint           call_count,
                      GError        **error)
{
  EvaStream *substream = EVA_STREAM (queue->read_streams->head->data);
  guint subrv = eva_stream_read_buffer (substream, out, error);
  *rv_inout += subrv;
  if (call_count > 0             /* only retry if this is the first time through */
   || *error)                    /* don't retry if an error occurred */
    return RES_STOP_TRYING;
  return eva_io_get_is_readable (substream)
       ? (subrv == 0 ? RES_STOP_TRYING : RES_TRY_AGAIN)
       : RES_DONE_WITH_STREAM;
}

static guint
eva_stream_queue_raw_read_buffer (EvaStream     *stream,
			          EvaBuffer     *buffer,
			          GError       **error)
{
  EvaStreamQueue *queue = EVA_STREAM_QUEUE (stream);
  unsigned rv = 0;
  queue->is_reading = 1;
  while (queue->read_streams->head != NULL)
    {
      guint count = 0;
      switch (try_one_read_buffer (queue, buffer, &rv, count++, error))
        {
        case RES_STOP_TRYING:
          goto done;
        case RES_DONE_WITH_STREAM:
          dequeue_write_stream (queue);
          break;
        case RES_TRY_AGAIN:
          break;
        }
    }
done:
  queue->is_reading = 0;
  if (should_be_write_shutdown (queue) && rv == 0)
    eva_io_notify_write_shutdown (EVA_IO (stream));
  return rv;
}

static Res   try_one_write (EvaStreamQueue *queue,
                            gconstpointer   data,
                            guint           length,
                            guint          *rv_inout,
                            guint           call_count,
                            GError        **error)
{
  EvaStream *substream = EVA_STREAM (queue->write_streams->head->data);
  guint max_len = length - (*rv_inout);
  guint subwrite = eva_stream_write (substream,
                                     (char *) data + (*rv_inout),
                                     length - (*rv_inout),
                                     error);
  *rv_inout += subwrite;
  if (call_count > 0             /* only retry if this is the first time through */
   || subwrite == max_len        /* don't retry if done */
   || *error)                    /* don't retry if an error occurred */
    return RES_STOP_TRYING;
  return eva_io_get_is_writable (substream) ? RES_TRY_AGAIN : RES_DONE_WITH_STREAM;
}

static guint
eva_stream_queue_raw_write   (EvaStream     *stream,
                              gconstpointer  data,
                              guint          length,
                              GError       **error)
{
  EvaStreamQueue *queue = EVA_STREAM_QUEUE (stream);
  unsigned rv = 0;
  queue->is_writing = 1;
  while (queue->write_streams->head != NULL)
    {
      guint count = 0;
      switch (try_one_write (queue, data, length, &rv, count++, error))
        {
        case RES_STOP_TRYING:
          goto done;
        case RES_DONE_WITH_STREAM:
          dequeue_write_stream (queue);
          break;
        case RES_TRY_AGAIN:
          break;
        }
    }
done:
  queue->is_writing = 0;
  if (should_be_write_shutdown (queue) && rv == 0)
    eva_io_notify_write_shutdown (EVA_IO (stream));
  return rv;
}

static Res
try_one_write_buffer  (EvaStreamQueue *queue,
                       EvaBuffer              *out,
                       guint                  *rv_inout,
                       guint                   call_count,
                       GError                **error)
{
  EvaStream *substream = EVA_STREAM (queue->write_streams->head->data);
  *rv_inout += eva_stream_write_buffer (substream, out, error);
  if (call_count > 0             /* only retry if this is the first time through */
   || *error)                    /* don't retry if an error occurred */
    return RES_STOP_TRYING;
  return eva_io_get_is_writable (substream) ? RES_TRY_AGAIN : RES_DONE_WITH_STREAM;
}

static guint
eva_stream_queue_raw_write_buffer (EvaStream     *stream,
			           EvaBuffer     *buffer,
			           GError       **error)
{
  EvaStreamQueue *queue = EVA_STREAM_QUEUE (stream);
  unsigned rv = 0;
  queue->is_writing = 1;
  while (queue->write_streams->head != NULL)
    {
      guint count = 0;
      switch (try_one_write_buffer (queue, buffer, &rv, count++, error))
        {
        case RES_STOP_TRYING:
          goto done;
        case RES_DONE_WITH_STREAM:
          dequeue_write_stream (queue);
          break;
        case RES_TRY_AGAIN:
          break;
        }
    }
done:
  queue->is_writing = 0;
  if (should_be_write_shutdown (queue) && rv == 0)
    eva_io_notify_write_shutdown (EVA_IO (stream));
  return rv;
}

static void
eva_stream_queue_finalize (GObject *object)
{
  EvaStreamQueue *queue = EVA_STREAM_QUEUE (object);
  if (queue->read_streams->head != NULL)
    {
      eva_stream_untrap_readable (queue->read_streams->head);
      g_list_foreach (queue->read_streams->head, (GFunc) g_object_unref, NULL);
    }
  if (queue->write_streams->head != NULL)
    {
      eva_stream_untrap_writable (queue->write_streams->head);
      g_list_foreach (queue->write_streams->head, (GFunc) g_object_unref, NULL);
    }
  eva_hook_destruct (READ_EMPTY_HOOK (queue));
  eva_hook_destruct (WRITE_EMPTY_HOOK (queue));
  parent_class->finalize (object);
}

static void
eva_stream_queue_init (EvaStreamQueue *queue)
{
  queue->read_streams = g_queue_new ();
  queue->write_streams = g_queue_new ();
  EVA_HOOK_INIT_NO_SHUTDOWN (queue, EvaStreamQueue,
                             read_empty_hook,
                             0, set_read_empty_poll);
  EVA_HOOK_INIT_NO_SHUTDOWN (queue, EvaStreamQueue,
                             write_empty_hook,
                             0, set_write_empty_poll);
}

static void
eva_stream_queue_class_init (EvaStreamQueueClass *class)
{
  EvaStreamClass *stream_class = EVA_STREAM_CLASS (class);
  EvaIOClass *io_class = EVA_IO_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  parent_class = g_type_class_peek_parent (class);
  io_class->shutdown_read = eva_stream_queue_shutdown_read;
  io_class->set_poll_read = eva_stream_queue_set_poll_read;
  io_class->shutdown_write = eva_stream_queue_shutdown_write;
  io_class->set_poll_write = eva_stream_queue_set_poll_write;
  stream_class->raw_read_buffer = eva_stream_queue_raw_read_buffer;
  stream_class->raw_read = eva_stream_queue_raw_read;
  stream_class->raw_write_buffer = eva_stream_queue_raw_write_buffer;
  stream_class->raw_write = eva_stream_queue_raw_write;
  object_class->finalize = eva_stream_queue_finalize;
  EVA_HOOK_CLASS_INIT (object_class, "read-empty", EvaStreamQueue, read_empty_hook);
  EVA_HOOK_CLASS_INIT (object_class, "write-empty", EvaStreamQueue, write_empty_hook);
}

GType eva_stream_queue_get_type()
{
  static GType stream_queue_type = 0;
  if (!stream_queue_type)
    {
      static const GTypeInfo stream_queue_info =
      {
	sizeof(EvaStreamQueueClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_stream_queue_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EvaStreamQueue),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_stream_queue_init,
	NULL		/* value_table */
      };
      stream_queue_type = g_type_register_static (EVA_TYPE_STREAM,
                                                  "EvaStreamQueue",
						  &stream_queue_info, 0);
    }
  return stream_queue_type;
}

/**
 * eva_stream_queue_new:
 * @is_readable: whether the read can have readable streamss
 * appended to it.
 * @is_writable: whether the read can have writable streamss
 * appended to it.
 *
 * Create a stream which can have other
 * streams queueenated to either its read or write ends.
 *
 * returns: the newly allocated stream.
 */
EvaStreamQueue *
eva_stream_queue_new (gboolean is_readable,
                      gboolean is_writable)
{
  EvaStreamQueue *queue = g_object_new (EVA_TYPE_STREAM_QUEUE, NULL);
  if (is_readable)
    {
      eva_io_mark_is_readable (queue);
      EVA_HOOK_SET_FLAG (READ_EMPTY_HOOK (queue), IS_AVAILABLE);
    }
  if (is_writable)
    {
      eva_io_mark_is_writable (queue);
      EVA_HOOK_SET_FLAG (WRITE_EMPTY_HOOK (queue), IS_AVAILABLE);
    }
  return queue;
}

/**
 * eva_stream_queue_append_read_stream:
 * @queue: the larger stream.
 * @substream: the stream whose data will be read by @queue.
 * Note that this function increments the ref-count on substream,
 * so you must g_object_unref() it also.
 *
 * Append a readable stream to a queue-stream.
 * It will be read in the order in which it was appended.
 */
void
eva_stream_queue_append_read_stream (EvaStreamQueue *queue,
			             EvaStream      *substream)
{
  g_return_if_fail (eva_stream_get_is_readable (EVA_STREAM (queue)));
  g_return_if_fail (EVA_IS_STREAM (substream));
  g_return_if_fail (!queue->no_more_read_streams);
  g_queue_push_tail (queue->read_streams, substream);
  g_object_ref (substream);

  if (queue->read_streams->length == 1)
    {
      eva_hook_clear_idle_notify (READ_EMPTY_HOOK (queue));
      if (eva_io_is_polling_for_read (queue))
        trap_head_read_stream (queue);
    }
}

/**
 * eva_stream_queue_no_more_read_streams:
 * @queue: the stream to which no further streams can be appended.
 *
 * Indicate that you are done added substreams to a queue-stream.
 * When the last stream shuts down for reading, and all the buffered
 * data has been read from queue, then this queue-stream
 * will shut down.
 */
void
eva_stream_queue_no_more_read_streams(EvaStreamQueue *queue)
{
  g_return_if_fail (!queue->no_more_read_streams);
  queue->no_more_read_streams = 1;
  if (queue->read_streams->length == 0)
    eva_hook_clear_idle_notify (READ_EMPTY_HOOK (queue));
  eva_hook_notify_shutdown (READ_EMPTY_HOOK (queue));
  if (should_be_read_shutdown (queue))
    eva_io_notify_read_shutdown (EVA_IO (queue));
  if (queue->read_streams->length == 0)
    eva_io_notify_read_shutdown (queue);
}

/**
 * eva_stream_queue_append_write_stream:
 * @queue: the larger stream.
 * @substream: the stream which will be written to by the @queue.
 * Note that this function increments the ref-count on substream,
 * so you must g_object_unref() it also.
 *
 * Append a writable stream to a queue-stream.
 * It will be written to in the order in which it was appended.
 */
void
eva_stream_queue_append_write_stream (EvaStreamQueue *queue,
			              EvaStream      *substream)
{
  g_return_if_fail (eva_stream_get_is_writable (EVA_STREAM (queue)));
  g_return_if_fail (EVA_IS_STREAM (substream));
  g_return_if_fail (!queue->no_more_write_streams);
  g_queue_push_tail (queue->write_streams, substream);
  g_object_ref (substream);

  if (queue->write_streams->length == 1)
    {
      eva_hook_clear_idle_notify (WRITE_EMPTY_HOOK (queue));
      if (eva_io_is_polling_for_write (queue))
        trap_head_write_stream (queue);
    }
}

/**
 * eva_stream_queue_no_more_write_streams:
 * @queue: the stream to which no further streams can be appended.
 *
 * Indicate that you are done added substreams to a queue-stream.
 * When the last stream shuts down for writeing, then this queue-stream
 * will shut down.
 */
void
eva_stream_queue_no_more_write_streams(EvaStreamQueue *queue)
{
  g_return_if_fail (!queue->no_more_write_streams);
  queue->no_more_write_streams = 1;
  if (queue->write_streams->length == 0)
    eva_hook_clear_idle_notify (WRITE_EMPTY_HOOK (queue));
  eva_hook_notify_shutdown (WRITE_EMPTY_HOOK (queue));
  if (should_be_write_shutdown (queue))
    eva_io_notify_write_shutdown (EVA_IO (queue));
  if (queue->write_streams->length == 0)
    eva_io_notify_write_shutdown (queue);
}
