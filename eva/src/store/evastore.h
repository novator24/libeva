#ifndef __EVA_STORE_H_
#define __EVA_STORE_H_

/*
 * GskStore -- an object store.
 *
 * GskStoreRequest -- a request for some operation on a key from a GskStore.
 *
 * GskStoreFormatEntry -- associates a GskStorageFormat with an integer ID
 * and a GType.
 */

/*
 * Notes: user is responsible for maintaining a compatible list of
 * format entries on all clients using the store, with stable IDs, etc.
 * (Otherwise, we would also have to serialize the GskStorageFormats
 * using a known GskStorageFormat, etc.; having a user-specified
 * list seems less of a security risk.  But there's nothing preventing
 * someone from writing a "meta-format" that loads other formats
 * dynamically, if that's what they want.  The XML format is already
 * pretty damn open, too.)
 *
 * To figure out which format to use, we start with the value's
 * leaf type and work upwards, looking for a matching format entry.
 */

#include <glib-object.h>
#include "../evastream.h"
#include "evavaluerequest.h"
#include "evastorageformat.h"
#include "evastreammap.h"

G_BEGIN_DECLS

typedef GObjectClass                GskStoreClass;
typedef struct _GskStore            GskStore;

typedef GskRequestClass             GskStoreRequestClass;
typedef struct _GskStoreRequest     GskStoreRequest;

typedef GObjectClass                GskStoreFormatEntryClass;
typedef struct _GskStoreFormatEntry GskStoreFormatEntry;

typedef enum
  {
    EVA_STORE_REQUEST_LOAD,
    EVA_STORE_REQUEST_SAVE,
    EVA_STORE_REQUEST_DELETE,
    EVA_STORE_REQUEST_EXISTS
  }
GskStoreRequestType;

/*
 * GskStore
 */

GType eva_store_get_type (void) G_GNUC_CONST;

#define EVA_TYPE_STORE (eva_store_get_type ())
#define EVA_STORE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_STORE, GskStore))
#define EVA_STORE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_STORE, GskStoreClass))
#define EVA_STORE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_STORE, GskStoreClass))
#define EVA_IS_STORE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_STORE))
#define EVA_IS_STORE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_STORE))

struct _GskStore
{
  GObject object;

  GskStreamMap *stream_map;
  GPtrArray *format_entries; /* of GskStoreFormatEntry */
};

GskStoreRequest * eva_store_save        (GskStore      *store,
					 const char    *key,
					 const GValue  *value,
					 GError       **error);

GskStoreRequest * eva_store_load        (GskStore      *store,
					 const char    *key,
					 GType          value_type,
					 GError       **error);

GskStoreRequest * eva_store_delete      (GskStore      *store,
					 const char    *key,
					 GError       **error);

GskStoreRequest * eva_store_exists      (GskStore      *store,
					 const char    *key,
					 GError       **error);

G_INLINE_FUNC
GskStoreRequest * eva_store_save_object (GskStore      *store,
					 const char    *key,
					 gpointer      object,
					 GError      **error);
/*
 * GskStoreRequest
 */

GType eva_store_request_get_type (void) G_GNUC_CONST;

#define EVA_TYPE_STORE_REQUEST (eva_store_request_get_type ())
#define EVA_STORE_REQUEST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
			       EVA_TYPE_STORE_REQUEST, \
			       GskStoreRequest))
#define EVA_STORE_REQUEST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
			    EVA_TYPE_STORE_REQUEST, \
			    GskStoreRequestClass))
#define EVA_STORE_REQUEST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
			      EVA_TYPE_STORE_REQUEST, \
			      GskStoreRequestClass))
#define EVA_IS_STORE_REQUEST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_STORE_REQUEST))
#define EVA_IS_STORE_REQUEST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_STORE_REQUEST))

struct _GskStoreRequest
{
  GskValueRequest value_request;

  GskStoreRequestType request_type;
  char *key;
  gpointer private;
};

G_INLINE_FUNC
const char * eva_store_request_get_key    (gpointer request);

G_INLINE_FUNC G_CONST_RETURN
GValue *     eva_store_request_get_value  (gpointer request);

G_INLINE_FUNC
gboolean     eva_store_request_get_exists (gpointer request);

G_INLINE_FUNC
gpointer     eva_store_request_get_object (gpointer request);

/*
 * GskStoreFormatEntry
 */

GType eva_store_format_entry_get_type (void) G_GNUC_CONST;

#define EVA_TYPE_STORE_FORMAT_ENTRY (eva_store_format_entry_get_type ())
#define EVA_STORE_FORMAT_ENTRY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
			       EVA_TYPE_STORE_FORMAT_ENTRY, \
			       GskStoreFormatEntry))
#define EVA_STORE_FORMAT_ENTRY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
			    EVA_TYPE_STORE_FORMAT_ENTRY, \
			    GskStoreFormatEntryClass))
#define EVA_STORE_FORMAT_ENTRY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
			      EVA_TYPE_STORE_FORMAT_ENTRY, \
			      GskStoreFormatEntryClass))
#define EVA_IS_STORE_FORMAT_ENTRY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_STORE_FORMAT_ENTRY))
#define EVA_IS_STORE_FORMAT_ENTRY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_STORE_FORMAT_ENTRY))

struct _GskStoreFormatEntry
{
  GObject object;

  guint32 format_id;
  GType value_type;
  GskStorageFormat *storage_format;
};

/*
 * Inline functions.
 */

#if defined (G_CAN_INLINE) || defined (__EVA_STORE_C__)

G_INLINE_FUNC GskStoreRequest *
eva_store_save_object (GskStore    *store,
		       const char  *key,
		       gpointer    object,
		       GError    **error)
{
  GValue value = { 0, { { 0 }, { 0 } } };
  GskStoreRequest *store_request;

  g_value_init (&value, G_OBJECT_TYPE (object));
  g_value_set_object (&value, object);
  store_request = eva_store_save (store, key, &value, error);
  g_value_unset (&value);
  return store_request;
}

G_INLINE_FUNC const char *
eva_store_request_get_key (gpointer ptr)
{
  GskStoreRequest *store_request = EVA_STORE_REQUEST (ptr);
  g_return_val_if_fail (store_request, FALSE);
  g_return_val_if_fail (EVA_IS_STORE_REQUEST (store_request), FALSE);
  return store_request->key;
}

G_INLINE_FUNC G_CONST_RETURN GValue *
eva_store_request_get_value (gpointer request)
{
  g_return_val_if_fail (request, NULL);
  g_return_val_if_fail (EVA_IS_STORE_REQUEST (request), NULL);
  g_return_val_if_fail (eva_request_get_is_done (request), NULL);
  g_return_val_if_fail (!eva_request_had_error (request), NULL);
  g_return_val_if_fail (EVA_STORE_REQUEST (request)->request_type ==
			  EVA_STORE_REQUEST_LOAD, NULL);
  return eva_value_request_get_value (request);
}

G_INLINE_FUNC gpointer
eva_store_request_get_object (gpointer request)
{
  const GValue *value;

  g_return_val_if_fail (request, NULL);
  g_return_val_if_fail (EVA_IS_STORE_REQUEST (request), NULL);
  g_return_val_if_fail (eva_request_get_is_done (request), NULL);
  g_return_val_if_fail (!eva_request_had_error (request), NULL);
  g_return_val_if_fail (EVA_STORE_REQUEST (request)->request_type ==
			  EVA_STORE_REQUEST_LOAD, NULL);

  value = eva_value_request_get_value (request);
  g_return_val_if_fail (value, NULL);
  g_return_val_if_fail (g_type_is_a (G_VALUE_TYPE (value), G_TYPE_OBJECT),
			NULL);

  return g_value_dup_object (value);
}

G_INLINE_FUNC gboolean
eva_store_request_get_exists (gpointer ptr)
{
  GskStoreRequest *store_request = EVA_STORE_REQUEST (ptr);
  g_return_val_if_fail (store_request, FALSE);
  g_return_val_if_fail (EVA_IS_STORE_REQUEST (store_request), FALSE);
  g_return_val_if_fail (eva_request_get_is_done (store_request), FALSE);
  g_return_val_if_fail (!eva_request_had_error (store_request), FALSE);
  g_return_val_if_fail (store_request->request_type == EVA_STORE_REQUEST_EXISTS,
			FALSE);
  return g_value_get_boolean (eva_value_request_get_value (store_request));
}

#endif /* defined (G_CAN_INLINE) || defined (__EVA_STORE_C__) */

G_END_DECLS

#endif
