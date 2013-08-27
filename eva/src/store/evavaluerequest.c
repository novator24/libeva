#define G_IMPLEMENT_INLINES
#define __EVA_VALUE_REQUEST_C__
#include "evavaluerequest.h"

static GObjectClass *parent_class = NULL;

/* GObject methods. */

static void
eva_value_request_finalize (GObject *object)
{
  EvaValueRequest *request = EVA_VALUE_REQUEST (object);
  if (G_VALUE_TYPE (&request->value))
    g_value_unset (&request->value);
  (*parent_class->finalize) (object);
}

static void
eva_value_request_class_init (EvaRequestClass *request_class)
{
  parent_class = g_type_class_peek_parent (request_class);
  G_OBJECT_CLASS (request_class)->finalize = eva_value_request_finalize;
}

GType
eva_value_request_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0))
    {
      static const GTypeInfo type_info =
	{
	  sizeof (EvaValueRequestClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) eva_value_request_class_init,
	  NULL,		/* class_finalize */
	  NULL,		/* class_data */
	  sizeof (EvaValueRequest),
	  0,		/* n_preallocs */
	  (GInstanceInitFunc) NULL,
	  NULL		/* value_table */
	};
      type = g_type_register_static (EVA_TYPE_REQUEST,
				     "EvaValueRequest",
				     &type_info,
				     0);
    }
  return type;
}
