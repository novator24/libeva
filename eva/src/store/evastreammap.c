#define G_IMPLEMENT_INLINES
#define __EVA_STREAM_MAP_C__
#include "evastreammap.h"

/*
 * EvaStreamMap
 */

GType
eva_stream_map_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0))
    {
      static const GTypeInfo type_info =
	{
	  sizeof (EvaStreamMapIface),
	  NULL, /* base_init */
	  NULL, /* base_finalize */
	  NULL,
	  NULL, /* class_finalize */
	  NULL, /* class_data */
	  0,
	  0,
	  NULL,
	  NULL
	};
      type = g_type_register_static (G_TYPE_INTERFACE,
				     "EvaStreamMap",
				     &type_info,
				     G_TYPE_FLAG_ABSTRACT);
    }
  return type;
}

/*
 * EvaStreamMapRequest
 */

static GObjectClass *eva_stream_map_request_parent_class = NULL;

static void
eva_stream_map_request_finalize (GObject *object)
{
  EvaStreamMapRequest *request = EVA_STREAM_MAP_REQUEST (object);
  if (request->key)
    g_free (request->key);
  (*eva_stream_map_request_parent_class->finalize) (object);
}

static void
eva_stream_map_request_class_init (EvaRequestClass *request_class)
{
  eva_stream_map_request_parent_class =
    g_type_class_peek_parent (request_class);
  G_OBJECT_CLASS (request_class)->finalize = eva_stream_map_request_finalize;
}

GType
eva_stream_map_request_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0))
    {
      static const GTypeInfo type_info =
	{
	  sizeof (EvaStreamMapRequestClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) eva_stream_map_request_class_init,
	  NULL,		/* class_finalize */
	  NULL,		/* class_data */
	  sizeof (EvaStreamMapRequest),
	  0,		/* n_preallocs */
	  (GInstanceInitFunc) NULL,
	  NULL		/* value_table */
	};
      type = g_type_register_static (EVA_TYPE_REQUEST,
				     "EvaStreamMapRequest",
				     &type_info,
				     0);
    }
  return type;
}
