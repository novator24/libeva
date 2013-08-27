#ifndef __EVA_STREAM_MAP_H_
#define __EVA_STREAM_MAP_H_

/*
 * GskStreamMap -- interface for a mapping between keys and streams,
 * like a very minimal filesystem.
 *
 * GskStreamMapRequest -- a request for some operation on a key
 * from a GskStreamMap.
 */

#include "../evastream.h"
#include "../evarequest.h"

G_BEGIN_DECLS

typedef struct _GskStreamMapIface   GskStreamMapIface;
typedef struct _GskStreamMap        GskStreamMap;

typedef GskRequestClass             GskStreamMapRequestClass;
typedef struct _GskStreamMapRequest GskStreamMapRequest;

typedef enum
  {
    EVA_STREAM_MAP_REQUEST_DELETE,
    EVA_STREAM_MAP_REQUEST_EXISTS

/* TODO: who owns the lock? the process? the caller...? */
#if 0
    EVA_STREAM_MAP_REQUEST_LOCK,
    EVA_STREAM_MAP_REQUEST_UNLOCK
#endif
  }
GskStreamMapRequestType;

/*
 * GskStreamMap
 */

GType eva_stream_map_get_type (void) G_GNUC_CONST;

#define EVA_TYPE_STREAM_MAP (eva_stream_map_get_type ())
#define EVA_STREAM_MAP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_STREAM_MAP, GskStreamMap))
#define EVA_STREAM_MAP_GET_IFACE(obj) \
  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), \
				  EVA_TYPE_STREAM_MAP, \
				  GskStreamMapIface))
#define EVA_IS_STREAM_MAP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_STREAM_MAP))

struct _GskStreamMapIface
{
  GTypeInterface base_iface;

  /* Return a read-only stream for key. */
  GskStream *           (*get)    (GskStreamMap  *map,
				   const char    *key,
				   GError       **error);

  /* Return a write-only stream for key; if the user writes data to the
   * stream and shuts it down without error, a stream containing the
   * same data must be the result of a subsequent get().
   */
  GskStream *           (*set)    (GskStreamMap  *map,
				   const char    *key,
				   GError       **error);

  GskStreamMapRequest * (*delete) (GskStreamMap  *map,
				   const char    *key,
				   GError       **error);

  GskStreamMapRequest * (*exists) (GskStreamMap  *map,
				   const char    *key,
				   GError       **error);

#if 0
  GskStreamMapRequest * (*lock)   (GskStreamMap  *map,
				   const char    *key,
				   GError       **error);

  GskStreamMapRequest * (*unlock) (GskStreamMap  *map,
				   const char    *key,
				   GError       **error);
#endif
};

/*
 * Convenience wrappers.
 */

G_INLINE_FUNC GskStream *           eva_stream_map_get    (gpointer     map,
							   const char  *key,
							   GError     **error);
G_INLINE_FUNC GskStream *           eva_stream_map_set    (gpointer     map,
							   const char  *key,
							   GError     **error);
G_INLINE_FUNC GskStreamMapRequest * eva_stream_map_delete (gpointer     map,
							   const char  *key,
							   GError     **error);
G_INLINE_FUNC GskStreamMapRequest * eva_stream_map_exists (gpointer     map,
							   const char  *key,
							   GError     **error);
#if 0
G_INLINE_FUNC GskStreamMapRequest * eva_stream_map_lock   (gpointer     map,
							   const char  *key,
							   GError     **error);
G_INLINE_FUNC GskStreamMapRequest * eva_stream_map_unlock (gpointer     map,
							   const char  *key,
							   GError     **error);
#endif

/*
 * GskStreamMapRequest
 */

GType eva_stream_map_request_get_type (void) G_GNUC_CONST;

#define EVA_TYPE_STREAM_MAP_REQUEST (eva_stream_map_request_get_type ())
#define EVA_STREAM_MAP_REQUEST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
			       EVA_TYPE_STREAM_MAP_REQUEST, \
			       GskStreamMapRequest))
#define EVA_STREAM_MAP_REQUEST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
			    EVA_TYPE_STREAM_MAP_REQUEST, \
			    GskStreamMapRequestClass))
#define EVA_STREAM_MAP_REQUEST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
			      EVA_TYPE_STREAM_MAP_REQUEST, \
			      GskStreamMapRequestClass))
#define EVA_IS_STREAM_MAP_REQUEST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_STREAM_MAP_REQUEST))
#define EVA_IS_STREAM_MAP_REQUEST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_STREAM_MAP_REQUEST))

struct _GskStreamMapRequest
{
  GskRequest request;

  GskStreamMapRequestType request_type;
  char *key;
  gboolean exists; /* iff request_type == EVA_STREAM_MAP_REQUEST_EXISTS */
};

G_INLINE_FUNC const char * eva_stream_map_request_get_key    (gpointer request);
G_INLINE_FUNC gboolean     eva_stream_map_request_get_exists (gpointer request);


/*
 * Inline functions.
 */

#if defined (G_CAN_INLINE) || defined (__EVA_STREAM_MAP_C__)

G_INLINE_FUNC GskStream *
eva_stream_map_get (gpointer ptr, const char *key, GError **error)
{
  GskStreamMap *map = EVA_STREAM_MAP (ptr);
  g_return_val_if_fail (map, NULL);
  g_return_val_if_fail (EVA_IS_STREAM_MAP (map), NULL);
  g_return_val_if_fail (key, NULL);
  g_return_val_if_fail (EVA_STREAM_MAP_GET_IFACE (map), NULL);
  g_return_val_if_fail (EVA_STREAM_MAP_GET_IFACE (map)->get, NULL);
  return (*EVA_STREAM_MAP_GET_IFACE (map)->get) (map, key, error);
}

G_INLINE_FUNC GskStream *
eva_stream_map_set (gpointer     ptr,
		    const char  *key,
		    GError     **error)
{
  GskStreamMap *map = EVA_STREAM_MAP (ptr);
  g_return_val_if_fail (map, NULL);
  g_return_val_if_fail (EVA_IS_STREAM_MAP (map), NULL);
  g_return_val_if_fail (key, NULL);
  g_return_val_if_fail (EVA_STREAM_MAP_GET_IFACE (map), NULL);
  g_return_val_if_fail (EVA_STREAM_MAP_GET_IFACE (map)->set, NULL);
  return (*EVA_STREAM_MAP_GET_IFACE (map)->set) (map, key, error);
}

G_INLINE_FUNC GskStreamMapRequest *
eva_stream_map_delete (gpointer ptr, const char *key, GError **error)
{
  GskStreamMap *map = EVA_STREAM_MAP (ptr);
  g_return_val_if_fail (map, NULL);
  g_return_val_if_fail (EVA_IS_STREAM_MAP (map), NULL);
  g_return_val_if_fail (key, NULL);
  g_return_val_if_fail (EVA_STREAM_MAP_GET_IFACE (map), NULL);
  g_return_val_if_fail (EVA_STREAM_MAP_GET_IFACE (map)->delete, NULL);
  return (*EVA_STREAM_MAP_GET_IFACE (map)->delete) (map, key, error);
}

G_INLINE_FUNC GskStreamMapRequest *
eva_stream_map_exists (gpointer ptr, const char *key, GError **error)
{
  GskStreamMap *map = EVA_STREAM_MAP (ptr);
  g_return_val_if_fail (map, NULL);
  g_return_val_if_fail (EVA_IS_STREAM_MAP (map), NULL);
  g_return_val_if_fail (key, NULL);
  g_return_val_if_fail (EVA_STREAM_MAP_GET_IFACE (map), NULL);
  g_return_val_if_fail (EVA_STREAM_MAP_GET_IFACE (map)->exists, NULL);
  return (*EVA_STREAM_MAP_GET_IFACE (map)->exists) (map, key, error);
}

#if 0
G_INLINE_FUNC GskStreamMapRequest *
eva_stream_map_lock (gpointer ptr, const char *key, GError **error)
{
  GskStreamMap *map = EVA_STREAM_MAP (ptr);
  g_return_val_if_fail (map, NULL);
  g_return_val_if_fail (EVA_IS_STREAM_MAP (map), NULL);
  g_return_val_if_fail (key, NULL);
  g_return_val_if_fail (EVA_STREAM_MAP_GET_IFACE (map), NULL);
  g_return_val_if_fail (EVA_STREAM_MAP_GET_IFACE (map)->lock, NULL);
  return (*EVA_STREAM_MAP_GET_IFACE (map)->lock) (map, key, error);
}

G_INLINE_FUNC GskStreamMapRequest *
eva_stream_map_unlock (gpointer ptr, const char *key, GError **error)
{
  GskStreamMap *map = EVA_STREAM_MAP (ptr);
  g_return_val_if_fail (map, NULL);
  g_return_val_if_fail (EVA_IS_STREAM_MAP (map), NULL);
  g_return_val_if_fail (key, NULL);
  g_return_val_if_fail (EVA_STREAM_MAP_GET_IFACE (map), NULL);
  g_return_val_if_fail (EVA_STREAM_MAP_GET_IFACE (map)->unlock, NULL);
  return (*EVA_STREAM_MAP_GET_IFACE (map)->unlock) (map, key, error);
}
#endif

G_INLINE_FUNC gboolean
eva_stream_map_request_get_exists (gpointer ptr)
{
  GskStreamMapRequest *request = EVA_STREAM_MAP_REQUEST (ptr);
  g_return_val_if_fail (request, FALSE);
  g_return_val_if_fail (EVA_IS_STREAM_MAP_REQUEST (request), FALSE);
  g_return_val_if_fail (eva_request_get_is_done (request), FALSE);
  g_return_val_if_fail (!eva_request_had_error (request), FALSE);
  g_return_val_if_fail (request->request_type == EVA_STREAM_MAP_REQUEST_EXISTS,
			FALSE);
  return request->exists;
}

G_INLINE_FUNC const char *
eva_stream_map_request_get_key (gpointer ptr)
{
  GskStreamMapRequest *request = EVA_STREAM_MAP_REQUEST (ptr);
  g_return_val_if_fail (request, NULL);
  g_return_val_if_fail (EVA_IS_STREAM_MAP_REQUEST (request), NULL);
  return request->key;
}

#endif /* defined (G_CAN_INLINE) || defined (__EVA_STREAM_MAP_C__) */

G_END_DECLS

#endif
