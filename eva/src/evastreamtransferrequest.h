#ifndef __EVA_STREAM_TRANSFER_REQUEST_H_
#define __EVA_STREAM_TRANSFER_REQUEST_H_

/*
 * EvaStreamTransferRequest -- basically just like a EvaStreamConnection,
 * except the requester gets notified when the read stream's data has been
 * fully transferred without error, or when an error occurs.
 */

/* TODO: might be useful to add a mode which only transfers n bytes,
 * and then doesn't shut down the streams.
 */

#include "evastream.h"
#include "evarequest.h"

G_BEGIN_DECLS

typedef EvaRequestClass                  EvaStreamTransferRequestClass;
typedef struct _EvaStreamTransferRequest EvaStreamTransferRequest;

GType eva_stream_transfer_request_get_type (void) G_GNUC_CONST;

#define EVA_TYPE_STREAM_TRANSFER_REQUEST \
  (eva_stream_transfer_request_get_type ())
#define EVA_STREAM_TRANSFER_REQUEST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
			       EVA_TYPE_STREAM_TRANSFER_REQUEST, \
			       EvaStreamTransferRequest))
#define EVA_IS_STREAM_TRANSFER_REQUEST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_STREAM_TRANSFER_REQUEST))

struct _EvaStreamTransferRequest
{
  EvaRequest request;

  /* The stream to read from. */
  EvaStream *read_side;

  /* The stream to write to. */
  EvaStream *write_side;

  /* Data which is to be transferred from read_side to write_side,
   * which hasn't been processed on the write side.
   */
  EvaBuffer buffer;

  /* The maximum number of bytes to store in buffer. */
  guint max_buffered;

  /* The maximum number of bytes to read atomically from the input stream. */
  guint atomic_read_size;

  /* Whether we are blocking the read-side because the buffer is 0 length. */
  guint blocking_write_side : 1;

  /* Whether we are blocking the write-side because the buffer is too long. */
  guint blocking_read_side : 1;
};

/* The request references both input_stream and output_stream
 * (i.e., you must also unref them at some point).
 */
EvaStreamTransferRequest *
       eva_stream_transfer_request_new
				  (EvaStream                *input_stream,
				   EvaStream                *output_stream);

void   eva_stream_transfer_request_set_max_buffered
				  (EvaStreamTransferRequest *request,
				   guint                     max_buffered);

guint  eva_stream_transfer_request_get_max_buffered
				  (EvaStreamTransferRequest *request);

void   eva_stream_transfer_request_set_atomic_read_size
				  (EvaStreamTransferRequest *request,
				   guint                     atomic_read_size);

guint  eva_stream_transfer_request_get_atomic_read_size
				  (EvaStreamTransferRequest *request);

G_END_DECLS

#endif
