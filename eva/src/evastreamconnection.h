#ifndef __EVA_STREAM_CONNECTION_H_
#define __EVA_STREAM_CONNECTION_H_

#include "evastream.h"

G_BEGIN_DECLS

typedef struct _EvaStreamConnection EvaStreamConnection;

GType eva_stream_connection_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_STREAM_CONNECTION	(eva_stream_connection_get_type ())
#define EVA_STREAM_CONNECTION(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_STREAM_CONNECTION, EvaStreamConnection))
#define EVA_IS_STREAM_CONNECTION(obj)   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_STREAM_CONNECTION))

EvaStreamConnection *
       eva_stream_connection_new              (EvaStream        *input_stream,
                                               EvaStream        *output_stream,
              		                       GError          **error);
void   eva_stream_connection_detach           (EvaStreamConnection *connection);
void   eva_stream_connection_shutdown         (EvaStreamConnection *connection);
void   eva_stream_connection_set_max_buffered (EvaStreamConnection *connection,
              		                       guint                max_buffered);
guint  eva_stream_connection_get_max_buffered (EvaStreamConnection *connection);
void   eva_stream_connection_set_atomic_read_size(EvaStreamConnection *connection,
              		                       guint                atomic_read_size);
guint  eva_stream_connection_get_atomic_read_size(EvaStreamConnection *connection);

#define eva_stream_connection_peek_read_side(conn) ((conn)->read_side)
#define eva_stream_connection_peek_write_side(conn) ((conn)->write_side)

/* private, but useful for debugging */
struct _EvaStreamConnection 
{
  GObject      object;

  /* The stream to read from. */
  EvaStream *read_side;

  /* The stream to write to. */
  EvaStream *write_side;

  /* Whether we are blocking the read-side because the buffer is 0 length. */
  guint blocking_write_side : 1;

  /* Whether we are blocking the write-side because the buffer is too long. */
  guint blocking_read_side : 1;

  /* Whether to use eva_stream_read_buffer() on the read-side.
     This is TRUE by default, even though it can cause
     both max_buffered and atomic_read_size to be violated. */
  guint use_read_buffer : 1;

  /* Data which is to be transferred from read_side to write_side,
     which hasn't been processed on the write side. */
  EvaBuffer buffer;

  /* The maximum number of bytes to store in buffer. */
  guint max_buffered;

  /* The maximum number of bytes to read atomically from the input stream. */
  guint atomic_read_size;
};

G_END_DECLS

#endif
