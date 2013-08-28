#ifndef __EVA_BUFFER_STREAM_H_
#define __EVA_BUFFER_STREAM_H_

#include "evastream.h"

/* A class to allow quick-and-dirty stream implementations.

   Instead of deriving from a EvaStream and doing a full
   implementation, you merely trap the buffer-read/buffer-write
   hooks and fill the buffers directly.

   Because the extensibility is attained through hooks
   you should NOT derive from this class, instead
   just add to the EvaBuffers directly.

   For implementing a stream using a buffer-stream,
   you should understand that the read_buffer
   is for putting data that will be eva_stream_read() out,
   and the write_buffer is for grabbing data
   that will had been eva_stream_write() in. */

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _EvaBufferStream EvaBufferStream;
typedef struct _EvaBufferStreamClass EvaBufferStreamClass;
/* --- type macros --- */
GType eva_buffer_stream_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_BUFFER_STREAM			(eva_buffer_stream_get_type ())
#define EVA_BUFFER_STREAM(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_BUFFER_STREAM, EvaBufferStream))
#define EVA_BUFFER_STREAM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_BUFFER_STREAM, EvaBufferStreamClass))
#define EVA_BUFFER_STREAM_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_BUFFER_STREAM, EvaBufferStreamClass))
#define EVA_IS_BUFFER_STREAM(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_BUFFER_STREAM))
#define EVA_IS_BUFFER_STREAM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_BUFFER_STREAM))

/* --- structures --- */
struct _EvaBufferStreamClass 		/* final */
{
  EvaStreamClass stream_class;

  void (*buffered_read_set_poll) (EvaBufferStream *bs, gboolean);
  void (*buffered_write_set_poll) (EvaBufferStream *bs, gboolean);
  void (*buffered_read_shutdown) (EvaBufferStream *bs);
  void (*buffered_write_shutdown) (EvaBufferStream *bs);
};
struct _EvaBufferStream 		/* final */
{
  EvaStream      stream; /*< private >*/

  /* after modifying any of these you
     must call eva_buffer_stream_changed()
     EXCEPT that
     you may call eva_buffer_stream_read_buffer_changed() instead
     if just the read_buffer was modified,
     and likewise 
     you may call eva_buffer_stream_write_buffer_changed() instead
     if just the write_buffer was modified, */

  EvaBuffer      read_buffer;
  EvaBuffer      write_buffer;

  /*< private >*/
  guint          max_write_buffer;

  /* Run when the read_buffer has been drained. */
  EvaHook        buffered_read_hook;

  /* Run when the write_buffer is non-empty. */
  EvaHook        buffered_write_hook;
};

/* --- prototypes --- */
EvaBufferStream *eva_buffer_stream_new (void);

void eva_buffer_stream_read_buffer_changed  (EvaBufferStream *stream);
void eva_buffer_stream_write_buffer_changed (EvaBufferStream *stream);
void eva_buffer_stream_changed              (EvaBufferStream *stream);

#define eva_buffer_stream_read_hook(stream)		\
	&(EVA_BUFFER_STREAM (stream)->buffered_read_hook)
#define eva_buffer_stream_write_hook(stream)		\
	&(EVA_BUFFER_STREAM (stream)->buffered_write_hook)

#define eva_buffer_stream_peek_read_buffer(stream)	\
	(&EVA_BUFFER_STREAM (stream)->read_buffer)
#define eva_buffer_stream_peek_write_buffer(stream)	\
	(&EVA_BUFFER_STREAM (stream)->write_buffer)
#define eva_buffer_stream_get_max_write_buffer(stream)	\
	(EVA_BUFFER_STREAM (stream)->max_write_buffer + 0)

/* whether to strictly enforce the max-write-buffer parameter */
#define eva_buffer_stream_has_strict_max_write(stream)	\
  EVA_HOOK_TEST_USER_FLAG (eva_buffer_stream_write_hook(stream), 1)
#define eva_buffer_stream_mark_strict_max_write(stream)	\
  EVA_HOOK_MARK_USER_FLAG (eva_buffer_stream_write_hook(stream), 1)
#define eva_buffer_stream_clear_strict_max_write(stream)	\
  EVA_HOOK_CLEAR_USER_FLAG (eva_buffer_stream_write_hook(stream), 1)

/* Shut the readable end of the stream down immediately
   if the buffer is empty, or shut it down when the buffer empties
   otherwise. */
void eva_buffer_stream_read_shutdown (EvaBufferStream *stream);

G_END_DECLS

#endif
