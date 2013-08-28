#ifndef __EVA_MIME_MULTIPART_DECODER_H_
#define __EVA_MIME_MULTIPART_DECODER_H_

/* Implements RFC 2046, Section 5: MIME MultipartDecoder Media Types */
#include "../evastream.h"
#include "evamimemultipartpiece.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _EvaMimeMultipartDecoder EvaMimeMultipartDecoder;
typedef struct _EvaMimeMultipartDecoderClass EvaMimeMultipartDecoderClass;

typedef gboolean (*EvaMimeMultipartDecoderHook) (EvaMimeMultipartDecoder *multipart_decoder);

/* --- type macros --- */
GType eva_mime_multipart_decoder_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_MIME_MULTIPART_DECODER			(eva_mime_multipart_decoder_get_type ())
#define EVA_MIME_MULTIPART_DECODER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_MIME_MULTIPART_DECODER, EvaMimeMultipartDecoder))
#define EVA_MIME_MULTIPART_DECODER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_MIME_MULTIPART_DECODER, EvaMimeMultipartDecoderClass))
#define EVA_MIME_MULTIPART_DECODER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_MIME_MULTIPART_DECODER, EvaMimeMultipartDecoderClass))
#define EVA_IS_MIME_MULTIPART_DECODER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_MIME_MULTIPART_DECODER))
#define EVA_IS_MIME_MULTIPART_DECODER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_MIME_MULTIPART_DECODER))

/* --- structures --- */

/* This controls whether large data is streamed out from the
   multipart_decoder.  The default is to put small data in memory
   and large data as streams. */
typedef enum
{
  EVA_MIME_MULTIPART_DECODER_MODE_DEFAULT,
  EVA_MIME_MULTIPART_DECODER_MODE_ALWAYS_MEMORY,
  EVA_MIME_MULTIPART_DECODER_MODE_ALWAYS_STREAM
} EvaMimeMultipartDecoderMode;

struct _EvaMimeMultipartDecoderClass 
{
  EvaStreamClass base_class;

  gboolean (*set_poll_new_part) (EvaMimeMultipartDecoder *decoder,
				 gboolean do_polling);
  void     (*shutdown_new_part) (EvaMimeMultipartDecoder *decoder);
};
struct _EvaMimeMultipartDecoder 
{
  EvaStream      base_instance;

  /* these are parsed out of the initial key-value pairs. */
  char *type;		/* 3.1: type of the body */
  char *start;		/* 3.2: content-id of root part */
  char *start_info;	/* 3.3: random application information */


  /*< private >*/
  EvaBuffer buffer;
  EvaHook new_part_available;
  GSList *first_piece;
  GSList *last_piece;

  guint is_shutdown : 1;
  guint swallowed_crlf : 1;

  /* The piece which is currently
     being parsed.
     This is non-NULL only if reading this piece. */
  EvaMimeMultipartPiece *current_piece;

  /* This EvaBufferStream is given encoded data:
     it is will be decoded then either given to the user directly,
     or if the data is being force into an in-memory buffer,
     it is handled by a eva_memory_buffer_sink, which eventually
     exports it into the buffer in current_piece.

     (Since it is a EvaBufferStream we can always write to it
     without blocking). */
  EvaStream *feed_stream;

  EvaMimeMultipartDecoderMode mode;
  guint8 feed_stream_encoding;
  char *boundary_str;
  unsigned boundary_str_len;

  guint n_pieces_alloced;
  guint n_pieces_obtained;
  guint next_piece_index_to_append;
  GHashTable *piece_index_to_piece;

  guint8 state;

  gboolean got_terminal_boundary;
};


/* --- prototypes --- */
EvaMimeMultipartDecoder *eva_mime_multipart_decoder_new       (char                       **kv_pairs);
void                   eva_mime_multipart_decoder_set_mode    (EvaMimeMultipartDecoder     *decoder,
						               EvaMimeMultipartDecoderMode  mode);
EvaMimeMultipartPiece *eva_mime_multipart_decoder_get_piece   (EvaMimeMultipartDecoder     *decoder);

#define eva_mime_multipart_decoder_trap(multipart_decoder, func, shutdown, data, destroy)	\
  eva_hook_trap (_EVA_MIME_MULTIPART_DECODER_HOOK(multipart_decoder),				\
		 (EvaHookFunc) func, (EvaHookFunc) shutdown, 			\
		 (data), (destroy))
#define eva_mime_multipart_decoder_untrap(multipart_decoder)  eva_hook_untrap (_EVA_MIME_MULTIPART_DECODER_HOOK(multipart_decoder))
#define eva_mime_multipart_decoder_block(multipart_decoder)   eva_hook_block (_EVA_MIME_MULTIPART_DECODER_HOOK(multipart_decoder))
#define eva_mime_multipart_decoder_unblock(multipart_decoder) eva_hook_unblock (_EVA_MIME_MULTIPART_DECODER_HOOK(multipart_decoder))

/* implementation bits */
/*< private >*/
#define _EVA_MIME_MULTIPART_DECODER_HOOK(multipart_decoder) (&(EVA_MIME_MULTIPART_DECODER (multipart_decoder)->new_part_available))

G_END_DECLS

#endif
