#include "evamimemultipartpiece.h"

/**
 * eva_mime_multipart_piece_alloc:
 *
 * Allocate a new MIME piece.
 *
 * returns: the newly allocated MIME piece.
 */
EvaMimeMultipartPiece *eva_mime_multipart_piece_alloc (void)
{
  EvaMimeMultipartPiece *rv = g_new0 (EvaMimeMultipartPiece, 1);
  rv->ref_count = 1;
  return rv;
}

/**
 * eva_mime_multipart_piece_ref:
 * @piece: a piece whose reference count should be increased by 1.
 *
 * Reference the piece.  This piece will only be destroyed
 * when its reference-count hits 0.
 *
 * returns: the @piece, for convenience.
 */
EvaMimeMultipartPiece *eva_mime_multipart_piece_ref   (EvaMimeMultipartPiece *piece)
{
  g_return_val_if_fail (piece->ref_count > 0, NULL);
  ++(piece->ref_count);
  return piece;
}

/**
 * eva_mime_multipart_piece_unref:
 * @piece: a piece whose reference count should be decreased by 1.
 *
 * Unreference the piece.  This piece will be destroyed
 * when its reference-count hits 0.
 */
void
eva_mime_multipart_piece_unref (EvaMimeMultipartPiece *piece)
{
  g_return_if_fail (piece->ref_count > 0);
  --(piece->ref_count);
  if (piece->ref_count == 0)
    {
      if (piece->is_memory)
	{
	  if (piece->destroy)
	    (*piece->destroy) (piece->destroy_data);
	}
      else
	{
	  if (piece->content)
	    g_object_unref (piece->content);
	}
      g_free (piece->type);
      g_free (piece->id);
      g_free (piece->description);
      g_free (piece->charset);
      g_free (piece->transfer_encoding);
      g_free (piece->disposition);
      g_free (piece);
    }
}

/**
 * eva_mime_multipart_piece_set_data:
 * @piece: the piece whose memory content should be set.
 * @data: the slab of memory to associate with this part.
 * @len: the length of the data in this part.
 * @destroy: function to invoke when this piece is destroyed.
 * @destroy_data: data to pass to @destroy.
 *
 * Set the binary data for a piece of a multipart stream,
 * with destroy notification.
 */
void
eva_mime_multipart_piece_set_data (EvaMimeMultipartPiece *piece,
				   gconstpointer          data,
				   guint                  len,
				   GDestroyNotify         destroy,
				   gpointer               destroy_data)
{
  g_return_if_fail (piece->content == NULL);
  g_return_if_fail (piece->is_memory == FALSE);
  piece->is_memory = 1;
  piece->content_data = data;
  piece->content_length = len;
  piece->destroy = destroy;
  piece->destroy_data = destroy_data;
}

/**
 * eva_mime_multipart_piece_set_stream:
 * @piece: the piece whose content-stream should be set.
 * @stream: the stream to associate with @piece.
 *
 * Set the content of a MIME piece to the @stream.
 */
void
eva_mime_multipart_piece_set_stream(EvaMimeMultipartPiece *piece,
				    EvaStream             *stream)
{
  g_return_if_fail (piece->content == NULL);
  g_return_if_fail (piece->is_memory == FALSE);
  g_return_if_fail (EVA_IS_STREAM (stream));
  piece->content = g_object_ref (stream);
}

/**
 * eva_mime_multipart_piece_set_description:
 * @piece: the piece to describe.
 * @description: the text description.
 *
 * Set the Content-Description tag for this MIME piece.
 */
void 
eva_mime_multipart_piece_set_description(EvaMimeMultipartPiece *piece,
					 const char            *description)
{
  g_free (piece->description);
  piece->description = g_strdup (description);
}

/**
 * eva_mime_multipart_piece_set_id:
 * @piece: the piece to describe.
 * @id: the content-id.
 *
 * Set the Content-ID tag for this MIME piece.
 *
 * See: XXX?
 */
void
eva_mime_multipart_piece_set_id    (EvaMimeMultipartPiece *piece,
				    const char            *id)
{
  g_free (piece->id);
  piece->id = g_strdup (id);
}

void
eva_mime_multipart_piece_set_location   (EvaMimeMultipartPiece *piece,
				         const char            *location)
{
  g_free (piece->location);
  piece->location = g_strdup (location);
}

/**
 * eva_mime_multipart_piece_set_transfer_encoding:
 * @piece: the piece to describe.
 * @encoding: the transfer-encoding
 *
 * Set the Content-Encoding tag for this MIME piece.
 *
 * Only three content-encodings are recognized:
 * "identity" (data transfered as binary data;
 * not safe for some mail gateways),
 * "base64" (binary data in 'base-64' encoding);
 * "quoted-printable" uses '=' to escape funny
 * characters as hex.
 */
void
eva_mime_multipart_piece_set_transfer_encoding
                                        (EvaMimeMultipartPiece *piece,
					 const char            *encoding)
{
  /* TODO: check that the encoding is valid. */
  g_free (piece->transfer_encoding);
  piece->transfer_encoding = g_strdup (encoding);
}

/**
 * eva_mime_multipart_piece_set_type:
 * @piece: the piece whose Content-Type header should be affected.
 * @type: the major type of this content, eg 'text' or 'image'.
 * @subtype: the minor type of this content, if type=='text',
 * then 'plain', 'html', 'wml' are common examples of subtypes.
 * @charset: the character set to use for text encodings.
 * @kv_pairs: any other key-value pairs are a NULL-terminated
 * array of strings, the even strings are keys, and the odd
 * strings are their values.
 *
 * Set the content-type for the given MIME piece.
 */
void
eva_mime_multipart_piece_set_type (EvaMimeMultipartPiece *piece,
				   const char            *type,
				   const char            *subtype,
				   const char            *charset,
				   const char * const    *kv_pairs)
{
  g_free (piece->type);
  g_free (piece->subtype);
  g_free (piece->charset);
  if (piece->other_fields)
    g_strfreev (piece->other_fields);

  piece->type = g_strdup (type);
  piece->subtype = g_strdup (subtype);
  piece->charset = g_strdup (charset);
  if (kv_pairs)
    piece->other_fields = g_strdupv ((char **) kv_pairs);
  else
    piece->other_fields = NULL;
}
