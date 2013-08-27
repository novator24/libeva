#include <stdio.h>
#include <errno.h>
#include "../evastreamfd.h"
#include "../url/evaurl.h"
#include "evafilestreammap.h"

static char *
make_filename (GskFileStreamMap *self, const char *key)
{
  char *encoded, *filename;

  g_return_val_if_fail (self->directory, NULL);
  encoded = eva_url_encode (key);
  g_return_val_if_fail (key, NULL);
  filename = g_strdup_printf ("%s/%s", self->directory, encoded);
  g_free (encoded);
  return filename;
}

/*
 *
 * GskFileStreamMapRequest
 *
 */

typedef GskStreamMapRequestClass        GskFileStreamMapRequestClass;
typedef struct _GskFileStreamMapRequest GskFileStreamMapRequest;

GType eva_file_stream_map_request_get_type (void) G_GNUC_CONST;

#define EVA_TYPE_FILE_STREAM_MAP_REQUEST \
  (eva_file_stream_map_request_get_type ())
#define EVA_FILE_STREAM_MAP_REQUEST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
			       EVA_TYPE_FILE_STREAM_MAP_REQUEST, \
			       GskFileStreamMapRequest))
#define EVA_IS_FILE_STREAM_MAP_REQUEST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_FILE_STREAM_MAP_REQUEST))

struct _GskFileStreamMapRequest
{
  GskStreamMapRequest stream_map_request;

  GskFileStreamMap *file_stream_map;
  GskStream *set_read_stream;
};

static GObjectClass *eva_file_stream_map_request_parent_class = NULL;

/* GskRequest methods. */

static void
eva_file_stream_map_request_start (GskRequest *request)
{
  GskFileStreamMapRequest *self = EVA_FILE_STREAM_MAP_REQUEST (request);
  GskFileStreamMap *file_stream_map = self->file_stream_map;
  char *key = self->stream_map_request.key;
  GError *error = NULL;
  char *filename;

  filename = make_filename (file_stream_map, key);
  g_return_if_fail (filename);

  switch (self->stream_map_request.request_type)
    {
      case EVA_STREAM_MAP_REQUEST_DELETE:
	{
	  if (remove (filename) != 0)
	    {
	      error = g_error_new (EVA_G_ERROR_DOMAIN,
				   eva_error_code_from_errno (errno),
				   "%s",
				   g_strerror (errno));
	      eva_request_set_error (self, error);
	    }
	  eva_request_done (self);
	}
	break;

      case EVA_STREAM_MAP_REQUEST_EXISTS:
	{
	  self->stream_map_request.exists =
	    g_file_test (filename, G_FILE_TEST_EXISTS);
	  eva_request_done (self);
	}
	break;
      default:
        g_return_if_reached ();
        break;
    }
  g_free (filename);
}

/* GObject methods. */

static void
eva_file_stream_map_request_finalize (GObject *object)
{
  GskFileStreamMapRequest *self = EVA_FILE_STREAM_MAP_REQUEST (object);

  if (self->file_stream_map)
    g_object_unref (self->file_stream_map);
  if (self->set_read_stream)
    g_object_unref (self->set_read_stream);

  (*eva_file_stream_map_request_parent_class->finalize) (object);
}

static void
eva_file_stream_map_request_class_init (GskRequestClass *request_class)
{
  eva_file_stream_map_request_parent_class =
    g_type_class_peek_parent (request_class);

  G_OBJECT_CLASS (request_class)->finalize =
    eva_file_stream_map_request_finalize;

  request_class->start = eva_file_stream_map_request_start;
}

GType
eva_file_stream_map_request_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0))
    {
      static const GTypeInfo type_info =
	{
	  sizeof (GskFileStreamMapRequestClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) eva_file_stream_map_request_class_init,
	  NULL,		/* class_finalize */
	  NULL,		/* class_data */
	  sizeof (GskFileStreamMapRequest),
	  0,		/* n_preallocs */
	  (GInstanceInitFunc) NULL,
	  NULL		/* value_table */
	};
      type = g_type_register_static (EVA_TYPE_STREAM_MAP_REQUEST,
				     "GskFileStreamMapRequest",
				     &type_info,
				     0);
    }
  return type;
}

static inline GskStreamMapRequest *
eva_file_stream_map_request_new (GskFileStreamMap        *file_stream_map,
				 const char              *key,
				 GskStreamMapRequestType  request_type)
{
  GskFileStreamMapRequest *request;

  g_return_val_if_fail (file_stream_map, NULL);
  g_return_val_if_fail (key, NULL);

  request = g_object_new (EVA_TYPE_FILE_STREAM_MAP_REQUEST, NULL);
  request->stream_map_request.request_type = request_type;
  request->stream_map_request.key = g_strdup (key);
  request->file_stream_map = file_stream_map;
  g_object_ref (file_stream_map);
  return EVA_STREAM_MAP_REQUEST (request);
}

/*
 * GskFileStreamMap
 */

static GObjectClass *eva_file_stream_map_parent_class = NULL;

/*
 * GskStreamMap interface.
 */

static GskStream *
eva_file_stream_map_get (GskStreamMap  *stream_map,
			 const char    *key,
			 GError       **error)
{
  GskFileStreamMap *self = EVA_FILE_STREAM_MAP (stream_map);
  char *filename;

  g_return_val_if_fail (key, NULL);
  filename = make_filename (self, key);
  g_return_val_if_fail (filename, NULL);
  return eva_stream_fd_new_read_file (filename, error);
}

static GskStream *
eva_file_stream_map_set (GskStreamMap *stream_map,
			 const char   *key,
			 GError      **error)
{
  GskFileStreamMap *self = EVA_FILE_STREAM_MAP (stream_map);
  char *filename;

  g_return_val_if_fail (key, NULL);
  filename = make_filename (self, key);
  g_return_val_if_fail (filename, NULL);
  return eva_stream_fd_new_write_file (filename, TRUE, TRUE, error);
}

static GskStreamMapRequest *
eva_file_stream_map_delete (GskStreamMap  *stream_map,
			    const char    *key,
			    GError       **error)
{
  (void) error;
  return eva_file_stream_map_request_new (EVA_FILE_STREAM_MAP (stream_map),
					  key,
					  EVA_STREAM_MAP_REQUEST_DELETE);
}

static GskStreamMapRequest *
eva_file_stream_map_exists (GskStreamMap  *stream_map,
			    const char    *key,
			    GError       **error)
{
  (void) error;
  return eva_file_stream_map_request_new (EVA_FILE_STREAM_MAP (stream_map),
					  key,
					  EVA_STREAM_MAP_REQUEST_EXISTS);
}

#if 0
static GskStreamMapRequest *
eva_file_stream_map_lock (GskStreamMap *map, const char *key, GError **error)
{
  /* TODO */
  (void) map;
  (void) key;
  (void) error;
  return NULL;
}

static GskStreamMapRequest *
eva_file_stream_map_unlock (GskStreamMap *map, const char *key, GError **error)
{
  /* TODO */
  (void) map;
  (void) key;
  (void) error;
  return NULL;
}
#endif

/*
 * GObject methods.
 */

static void
eva_file_stream_map_finalize (GObject *object)
{
  GskFileStreamMap *self = EVA_FILE_STREAM_MAP (object);

  if (self->directory)
    g_free (self->directory);

  (*eva_file_stream_map_parent_class->finalize) (object);
}

static void
eva_file_stream_map_stream_map_init (GskStreamMapIface *stream_map_iface)
{
  stream_map_iface->get = eva_file_stream_map_get;
  stream_map_iface->set = eva_file_stream_map_set;
  stream_map_iface->delete = eva_file_stream_map_delete;
  stream_map_iface->exists = eva_file_stream_map_exists;
#if 0
  stream_map_iface->lock = eva_file_stream_map_lock;
  stream_map_iface->unlock = eva_file_stream_map_unlock;
#endif
}

static void
eva_file_stream_map_class_init (GObjectClass *object_class)
{
  eva_file_stream_map_parent_class = g_type_class_peek_parent (object_class);
  object_class->finalize = eva_file_stream_map_finalize;
}

GType
eva_file_stream_map_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0))
    {
      static const GInterfaceInfo stream_map_info =
	{
	  (GInterfaceInitFunc) eva_file_stream_map_stream_map_init,
	  NULL,			/* interface_finalize */
	  NULL			/* interface_data */
	};
      static const GTypeInfo info =
	{
	  sizeof (GskFileStreamMapClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) eva_file_stream_map_class_init,
	  NULL,		/* class_finalize */
	  NULL,		/* class_data */
	  sizeof (GskFileStreamMap),
	  0,		/* n_preallocs */
	  (GInstanceInitFunc) NULL,
	  NULL		/* value_table */
	};
      type = g_type_register_static (G_TYPE_OBJECT,
				     "GskFileStreamMap",
				     &info,
				     0);
      g_type_add_interface_static (type,
				   EVA_TYPE_STREAM_MAP,
				   &stream_map_info);
    }
  return type;
}

GskFileStreamMap *
eva_file_stream_map_new (const char *directory)
{
  GskFileStreamMap *file_stream_map;

  file_stream_map = g_object_new (EVA_TYPE_FILE_STREAM_MAP, NULL);
  file_stream_map->directory = g_strdup (directory);
  return file_stream_map;
}
