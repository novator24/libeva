#ifndef __EVA_STREAM_H_
#define __EVA_STREAM_H_

#include "evabuffer.h"
#include "evaio.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _GskStream GskStream;
typedef struct _GskStreamClass GskStreamClass;

/* --- type macros --- */
GType eva_stream_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_STREAM			(eva_stream_get_type ())
#define EVA_STREAM(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_STREAM, GskStream))
#define EVA_STREAM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_STREAM, GskStreamClass))
#define EVA_STREAM_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_STREAM, GskStreamClass))
#define EVA_IS_STREAM(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_STREAM))
#define EVA_IS_STREAM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_STREAM))


/* Convention:
 *
 *
 *      The ends of a stream are the 'read-end' and the 'write-end'.
 *      The read-end is drawn-from using eva_stream_read()
 *      and the write-end is affected by eva_stream_write().
 *      
 *      Note that it might be tempting to call the 'read-end' the "output end",
 *      since that is where data is "output" -- that's confusing though --
 *      just avoid reference to output if you want to understand what's
 *      going on.  Likewise thinking of the 'write-end' as "for input" is more
 *      likely to confuse than clarify.
 */

/* --- structures --- */
struct _GskStreamClass 
{
  GskIOClass base_class;

  /* --- virtuals --- */
  guint      (*raw_read)        (GskStream     *stream,
			 	 gpointer       data,
			 	 guint          length,
			 	 GError       **error);
  guint      (*raw_write)       (GskStream     *stream,
			 	 gconstpointer  data,
			 	 guint          length,
			 	 GError       **error);
  guint      (*raw_read_buffer) (GskStream     *stream,
				 GskBuffer     *buffer,
				 GError       **error);
  guint      (*raw_write_buffer)(GskStream    *stream,
				 GskBuffer     *buffer,
				 GError       **error);
};

struct _GskStream 
{
  GskIO        base_io;

  /*< protected >*/
  guint        never_partial_reads : 1;
  guint        never_partial_writes : 1;
};


/* --- prototypes --- */

/* read-from/write-to a stream */
gsize    eva_stream_read              (GskStream        *stream,
		                       gpointer          buffer,
		                       gsize             buffer_length,
		                       GError          **error);
gsize    eva_stream_write             (GskStream        *stream,
		                       gconstpointer     buffer,
		                       gsize             buffer_length,
		                       GError          **error);
/* read into buffer from stream */
gsize    eva_stream_read_buffer       (GskStream        *stream,
		                       GskBuffer        *buffer,
		                       GError          **error);
/* write out of buffer to stream */
gsize    eva_stream_write_buffer      (GskStream        *stream,
		                       GskBuffer        *buffer,
		                       GError          **error);

/* connections from the output of one stream to the input of another. */
gboolean eva_stream_attach            (GskStream        *input_stream,
                                       GskStream        *output_stream,
				       GError          **error);

gboolean eva_stream_attach_pair       (GskStream        *stream_a,
                                       GskStream        *stream_b,
				       GError          **error);

/* public */
#define eva_stream_get_is_connecting(stream)                eva_io_get_is_connecting(stream)
#define eva_stream_get_is_readable(stream)                  eva_io_get_is_readable(stream)
#define eva_stream_get_is_writable(stream)                  eva_io_get_is_writable(stream)
#define eva_stream_get_never_blocks_write(stream)           eva_io_get_never_blocks_write(stream)
#define eva_stream_get_never_blocks_read(stream)            eva_io_get_never_blocks_read(stream)
#define eva_stream_get_idle_notify_write(stream)            eva_io_get_idle_notify_write(stream)
#define eva_stream_get_idle_notify_read(stream)             eva_io_get_idle_notify_read(stream)
#define eva_stream_get_is_open(stream)                      eva_io_get_is_open (stream)
#define eva_stream_get_never_partial_reads(stream)          (EVA_STREAM (stream)->never_partial_reads != 0)
#define eva_stream_get_never_partial_writes(stream)         (EVA_STREAM (stream)->never_partial_writes != 0)
#define eva_stream_trap_readable                            eva_io_trap_readable
#define eva_stream_trap_writable                            eva_io_trap_writable
#define eva_stream_untrap_readable                          eva_io_untrap_readable
#define eva_stream_untrap_writable                          eva_io_untrap_writable

/* protected */
#define eva_stream_mark_is_connecting(stream)               eva_io_mark_is_connecting(stream)
#define eva_stream_mark_is_readable(stream)                 eva_io_mark_is_readable(stream)
#define eva_stream_mark_is_writable(stream)                 eva_io_mark_is_writable(stream)
#define eva_stream_mark_never_blocks_write(stream)          eva_io_mark_never_blocks_write(stream)
#define eva_stream_mark_never_blocks_read(stream)           eva_io_mark_never_blocks_read(stream)
#define eva_stream_mark_idle_notify_write(stream)           eva_io_mark_idle_notify_write(stream)
#define eva_stream_mark_idle_notify_read(stream)            eva_io_mark_idle_notify_read(stream)
#define eva_stream_mark_is_open(stream)                     eva_io_mark_is_open (stream)
#define eva_stream_mark_never_partial_reads(stream)         G_STMT_START{ EVA_STREAM (stream)->never_partial_reads = 1; }G_STMT_END
#define eva_stream_mark_never_partial_writes(stream)        G_STMT_START{ EVA_STREAM (stream)->never_partial_writes = 1; }G_STMT_END
#define eva_stream_clear_is_readable(stream)                eva_io_clear_is_readable(stream)
#define eva_stream_clear_is_writable(stream)                eva_io_clear_is_writable(stream)
#define eva_stream_clear_is_open(stream)                    eva_io_clear_is_open (stream)
#define eva_stream_clear_idle_notify_write(stream)          eva_io_clear_idle_notify_write(stream)
#define eva_stream_clear_idle_notify_read(stream)           eva_io_clear_idle_notify_read(stream)
#define eva_stream_clear_never_partial_reads(stream)        G_STMT_START{ EVA_STREAM (stream)->never_partial_reads = 0; }G_STMT_END
#define eva_stream_clear_never_partial_writes(stream)       G_STMT_START{ EVA_STREAM (stream)->never_partial_writes = 0; }G_STMT_END

G_END_DECLS

#endif
