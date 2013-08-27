#ifndef __EVA_STREAM_CONCAT_H_
#define __EVA_STREAM_CONCAT_H_

#include "evastream.h"

G_BEGIN_DECLS

GskStream *eva_streams_concat_and_unref (GskStream *stream0,
                                         ...);
GskStream *eva_streams_concat_v         (unsigned    n_streams,
                                         GskStream **streams);
G_END_DECLS

#endif
