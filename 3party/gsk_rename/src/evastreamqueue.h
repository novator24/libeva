/* EvaStreamQueue: a container class for streams.
   you can add input or output streams and they will be read/written to
   until they shutdown.
   
   This class can also be used as a proxy. */

#ifndef __EVA_STREAM_QUEUE_H_
#define __EVA_STREAM_QUEUE_H_

#include "evastream.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _EvaStreamQueue EvaStreamQueue;
typedef struct _EvaStreamQueueClass EvaStreamQueueClass;
/* --- type macros --- */
GType eva_stream_queue_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_STREAM_QUEUE			(eva_stream_queue_get_type ())
#define EVA_STREAM_QUEUE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_STREAM_QUEUE, EvaStreamQueue))
#define EVA_STREAM_QUEUE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_STREAM_QUEUE, EvaStreamQueueClass))
#define EVA_STREAM_QUEUE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_STREAM_QUEUE, EvaStreamQueueClass))
#define EVA_IS_STREAM_QUEUE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_STREAM_QUEUE))
#define EVA_IS_STREAM_QUEUE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_STREAM_QUEUE))

/* --- structures --- */
struct _EvaStreamQueueClass 
{
  EvaStreamClass base_class;
  void (*set_read_empty_poll) (EvaStreamQueue *, gboolean);
  void (*set_write_empty_poll) (EvaStreamQueue *, gboolean);
};
struct _EvaStreamQueue 
{
  EvaStream      base_instance;

  /* a list of readable streams; the head of the queue will
     be removed once they reached end-of-file. */
  GQueue        *read_streams;
  GQueue        *write_streams;
  gboolean       no_more_read_streams;
  gboolean       no_more_write_streams;
  gboolean       is_reading, is_writing;

  /* These hooks are tripped whenever there are no more
     streams (the read_empty_hook is triggered
     when there are no more streams to read from;
     the write_empty_hook is triggered
     when there are no more streams to write to) */
  EvaHook        read_empty_hook;
  EvaHook        write_empty_hook;
};

/* --- prototypes --- */
EvaStreamQueue * eva_stream_queue_new       (gboolean is_readable,
                                             gboolean is_writable);

void eva_stream_queue_append_read_stream    (EvaStreamQueue *queue,
				             EvaStream      *substream);
void eva_stream_queue_append_write_stream   (EvaStreamQueue *queue,
				             EvaStream      *substream);
void eva_stream_queue_no_more_read_streams  (EvaStreamQueue *queue);
void eva_stream_queue_no_more_write_streams (EvaStreamQueue *queue);

/* --- trapping the 'is_empty' event --- */
#define EVA_STREAM_QUEUE_READ_EMPTY_HOOK(queue)                                \
    (&(EVA_STREAM_QUEUE (queue)->read_empty_hook))
#define eva_stream_queue_trap_read_empty(queue, func, shutdown, data, destroy) \
  eva_hook_trap(EVA_STREAM_QUEUE_READ_EMPTY_HOOK(queue),                       \
		(EvaHookFunc) func, (EvaHookFunc) shutdown, data, destroy)
#define eva_stream_queue_untrap_read_empty(queue)                              \
  eva_hook_untrap(EVA_STREAM_QUEUE_READ_EMPTY_HOOK(queue))
#define EVA_STREAM_QUEUE_WRITE_EMPTY_HOOK(queue)                               \
    (&(EVA_STREAM_QUEUE (queue)->write_empty_hook))
#define eva_stream_queue_trap_write_empty(queue, func, shutdown, data, destroy)\
  eva_hook_trap(EVA_STREAM_QUEUE_WRITE_EMPTY_HOOK(queue), \
		(EvaHookFunc) func, (EvaHookFunc) shutdown, data, destroy)
#define eva_stream_queue_untrap_write_empty(queue) \
  eva_hook_untrap(EVA_STREAM_QUEUE_WRITE_EMPTY_HOOK(queue))


G_END_DECLS

#endif
