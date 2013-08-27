#ifndef __EVA_MIME_MULTIPART_ENCODER_H_
#define __EVA_MIME_MULTIPART_ENCODER_H_

#include "evamimemultipartpiece.h"
#include "../evastream.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _GskMimeMultipartEncoder GskMimeMultipartEncoder;
typedef struct _GskMimeMultipartEncoderClass GskMimeMultipartEncoderClass;
/* --- type macros --- */
GType eva_mime_multipart_encoder_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_MIME_MULTIPART_ENCODER			(eva_mime_multipart_encoder_get_type ())
#define EVA_MIME_MULTIPART_ENCODER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_MIME_MULTIPART_ENCODER, GskMimeMultipartEncoder))
#define EVA_MIME_MULTIPART_ENCODER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_MIME_MULTIPART_ENCODER, GskMimeMultipartEncoderClass))
#define EVA_MIME_MULTIPART_ENCODER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_MIME_MULTIPART_ENCODER, GskMimeMultipartEncoderClass))
#define EVA_IS_MIME_MULTIPART_ENCODER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_MIME_MULTIPART_ENCODER))
#define EVA_IS_MIME_MULTIPART_ENCODER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_MIME_MULTIPART_ENCODER))

/* --- structures --- */
struct _GskMimeMultipartEncoderClass 
{
  GskStreamClass stream_class;
  void (*new_part_needed_set_poll) (GskMimeMultipartEncoder  *encoder,
				    gboolean                  do_poll);
  void (*new_part_needed_shutdown) (GskMimeMultipartEncoder  *encoder);
};

struct _GskMimeMultipartEncoder 
{
  GskStream      stream;
  GskHook        new_part_needed;
  GQueue        *outgoing_pieces;
  GskStream     *active_stream;
  GskBuffer      outgoing_data;

  char          *boundary_str;

  guint          max_buffered;
  guint          blocked_active_stream : 1;
  guint          shutdown : 1;
  guint          wrote_terminator : 1;
};

/* --- prototypes --- */
#define EVA_MIME_MULTIPART_ENCODER_GOOD_BOUNDARY	"_=_"

GskMimeMultipartEncoder *eva_mime_multipart_encoder_new (const char *boundary);
gboolean eva_mime_multipart_encoder_add_part (GskMimeMultipartEncoder *encoder,
					      GskMimeMultipartPiece   *piece,
					      GError                 **error);

#define eva_mime_multipart_encoder_new_defaults()	                 \
  eva_mime_multipart_encoder_new(EVA_MIME_MULTIPART_ENCODER_GOOD_BOUNDARY)
#define eva_mime_multipart_encoder_trap_part_needed(encoder, func, data) \
  eva_hook_trap(_EVA_MIME_MULTIPART_ENCODER_HOOK(encoder),               \
		(GskHookFunc) (func), (data))
#define  eva_mime_multipart_encoder_terminate(encoder)			 \
  eva_hook_shutdown(_EVA_MIME_MULTIPART_ENCODER_HOOK(encoder), NULL)

/*< private >*/
#define _EVA_MIME_MULTIPART_ENCODER_HOOK(encoder)			 \
  (&((EVA_MIME_MULTIPART_ENCODER(encoder))->new_part_needed))

G_END_DECLS

#endif
