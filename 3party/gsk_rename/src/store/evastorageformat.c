#define G_IMPLEMENT_INLINES
#define __EVA_STORAGE_FORMAT_C__
#include "evastorageformat.h"

GType
eva_storage_format_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0))
    {
      static const GTypeInfo info =
	{
	  sizeof (EvaStorageFormatIface),
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
				     "EvaStorageFormat",
				     &info,
				     G_TYPE_FLAG_ABSTRACT);
      g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
    }
  return type;
}
