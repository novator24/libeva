#ifndef __EVA_STREAM_CONCAT_H_
#define __EVA_STREAM_CONCAT_H_

#include "evastream.h"

G_BEGIN_DECLS

EvaStream *eva_streams_concat_and_unref (EvaStream *stream0,
                                         ...);
EvaStream *eva_streams_concat_v         (unsigned    n_streams,
                                         EvaStream **streams);
G_END_DECLS

#endif
