#ifndef __EVA_MIME_MULTIPART_PIECE_H_
#define __EVA_MIME_MULTIPART_PIECE_H_

#include "../evastream.h"

G_BEGIN_DECLS

typedef struct _EvaMimeMultipartPiece EvaMimeMultipartPiece;
struct _EvaMimeMultipartPiece
{
  char *type;
  char *subtype;
  char *id;
  char *description;
  char *charset;
  char *location;
  char *transfer_encoding;
  char *disposition;
  char **other_fields;

  /* if is_memory */
  guint content_length;
  gconstpointer content_data;
  GDestroyNotify destroy;	/*< private >*/
  gpointer destroy_data;	/*< private >*/

  /* if !is_memory */
  EvaStream *content;

  guint16 is_memory : 1;

  /*< private >*/
  guint16 ref_count;
};

EvaMimeMultipartPiece *eva_mime_multipart_piece_alloc (void);
EvaMimeMultipartPiece *eva_mime_multipart_piece_ref   (EvaMimeMultipartPiece *piece);
void                   eva_mime_multipart_piece_unref (EvaMimeMultipartPiece *piece);

void  eva_mime_multipart_piece_set_data (EvaMimeMultipartPiece *piece,
					 gconstpointer          data,
					 guint                  len,
					 GDestroyNotify         destroy,
					 gpointer               destroy_data);
void
eva_mime_multipart_piece_set_stream     (EvaMimeMultipartPiece *piece,
				         EvaStream             *stream);
void 
eva_mime_multipart_piece_set_description(EvaMimeMultipartPiece *piece,
					 const char            *description);
void eva_mime_multipart_piece_set_id    (EvaMimeMultipartPiece *piece,
				         const char            *id);
void
eva_mime_multipart_piece_set_location   (EvaMimeMultipartPiece *piece,
				         const char            *location);
void eva_mime_multipart_piece_set_transfer_encoding
                                        (EvaMimeMultipartPiece *piece,
					 const char            *encoding);
void eva_mime_multipart_piece_set_type  (EvaMimeMultipartPiece *piece,
				         const char            *type,
					 const char            *subtype,
					 const char            *charset,
					 const char * const    *kv_pairs);




G_END_DECLS

#endif
