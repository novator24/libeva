#ifndef __EVA_STORAGE_FORMAT_H_
#define __EVA_STORAGE_FORMAT_H_

/*
 * GskStorageFormat -- interface for things that can serialize/deserialize
 * values to/from streams.
 */

#include "../evastream.h"
#include "evavaluerequest.h"

G_BEGIN_DECLS

typedef struct _GskStorageFormatIface GskStorageFormatIface;
typedef struct _GskStorageFormat      GskStorageFormat;

GType eva_storage_format_get_type (void) G_GNUC_CONST;

#define EVA_TYPE_STORAGE_FORMAT (eva_storage_format_get_type ())
#define EVA_STORAGE_FORMAT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
			       EVA_TYPE_STORAGE_FORMAT, \
			       GskStorageFormat))
#define EVA_STORAGE_FORMAT_GET_IFACE(obj) \
  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), \
				  EVA_TYPE_STORAGE_FORMAT, \
				  GskStorageFormatIface))
#define EVA_IS_STORAGE_FORMAT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_STORAGE_FORMAT))

struct _GskStorageFormatIface
{
  GTypeInterface base_iface;

  /* Return a read-only stream that can be deserialized into an
   * equivalent value.
   */
  GskStream *       (*serialize)   (GskStorageFormat  *format,
				    const GValue      *value,
				    GError           **error);

  /* Return a request to deserialize a value of type value_type from
   * a read-only stream.
   */
  GskValueRequest * (*deserialize) (GskStorageFormat  *format,
				    GskStream         *stream,
				    GType              value_type,
				    GError           **error);
};

/*
 * Convenience wrappers.
 */

G_INLINE_FUNC
GskStream *       eva_storage_format_serialize   (gpointer       format,
						  const GValue  *value,
						  GError       **error);
G_INLINE_FUNC
GskValueRequest * eva_storage_format_deserialize (gpointer       format,
						  GskStream     *stream,
						  GType          value_type,
						  GError       **error);

/*
 * Inline functions.
 */

#if defined (G_CAN_INLINE) || defined (__EVA_STORAGE_FORMAT_C__)

G_INLINE_FUNC GskStream *
eva_storage_format_serialize (gpointer       format,
			      const GValue  *value,
			      GError       **error)
{
  GskStorageFormatIface *iface;

  g_return_val_if_fail (format, NULL);
  g_return_val_if_fail (EVA_IS_STORAGE_FORMAT (format), NULL);
  iface = EVA_STORAGE_FORMAT_GET_IFACE (format);
  g_return_val_if_fail (iface, NULL);
  g_return_val_if_fail (iface->serialize, NULL);

  return (*iface->serialize) (EVA_STORAGE_FORMAT (format), value, error);
}

G_INLINE_FUNC GskValueRequest *
eva_storage_format_deserialize (gpointer    format,
				GskStream  *stream,
				GType       value_type,
				GError    **error)
{
  GskStorageFormatIface *iface;

  g_return_val_if_fail (format, NULL);
  g_return_val_if_fail (EVA_IS_STORAGE_FORMAT (format), NULL);
  iface = EVA_STORAGE_FORMAT_GET_IFACE (format);
  g_return_val_if_fail (iface, NULL);
  g_return_val_if_fail (iface->deserialize, NULL);

  return (*iface->deserialize) (EVA_STORAGE_FORMAT (format),
				stream,
				value_type,
				error);
}
#endif /* defined (G_CAN_INLINE) || defined (__EVA_STORAGE_FORMAT_C__) */

G_END_DECLS

#endif
