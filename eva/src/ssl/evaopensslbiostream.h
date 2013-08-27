#ifndef __EVA_OPENSSL_BIO_STREAM_PAIR_H_
#define __EVA_OPENSSL_BIO_STREAM_PAIR_H_

#include "../evabufferstream.h"
#include <openssl/ssl.h>

G_BEGIN_DECLS

/* Create a linked BIO and stream, such that reading the BIO
 * causes data to be read from the stream,
 * and data written to the stream causes the bio to be readable
 * and data written to the bio causes the stream to be readable.
 */
gboolean eva_openssl_bio_stream_pair (BIO             **bio_out,
                                      GskBufferStream **stream_out);

G_END_DECLS

#endif
