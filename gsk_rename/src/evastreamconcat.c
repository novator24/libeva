#include "evastreamqueue.h"
#include "evastreamconcat.h"

EvaStream *
eva_streams_concat_and_unref (EvaStream *stream0,
			      ...)
{
  EvaStreamQueue *queue = eva_stream_queue_new (TRUE, FALSE);
  EvaStream *stream;
  va_list args;
  va_start (args, stream0);
  for (stream = stream0; stream != NULL; stream = va_arg (args, EvaStream *))
    {
      eva_stream_queue_append_read_stream (queue, stream);
      g_object_unref (stream);
    }
  va_end (args);
  eva_stream_queue_no_more_read_streams (queue);
  return EVA_STREAM (queue);
}

EvaStream *
eva_streams_concat_v       (unsigned    n_streams,
                            EvaStream **streams)
{
  EvaStreamQueue *queue = eva_stream_queue_new (TRUE, FALSE);
  unsigned i;
  for (i = 0; i < n_streams; i++)
    eva_stream_queue_append_read_stream (queue, streams[i]);
  eva_stream_queue_no_more_read_streams (queue);
  return EVA_STREAM (queue);
}
