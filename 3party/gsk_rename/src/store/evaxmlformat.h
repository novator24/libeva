#ifndef __EVA_XML_FORMAT_H_
#define __EVA_XML_FORMAT_H_

/*
 * EvaXmlFormat -- implements EvaStorageFormat, uses EvaXmlValueReader
 * and EvaXmlValueWriter to deserialize/serialize values from/to XML.
 */

#include "evastorageformat.h"
#include "evagtypeloader.h"

G_BEGIN_DECLS

typedef GObjectClass         EvaXmlFormatClass;
typedef struct _EvaXmlFormat EvaXmlFormat;

GType eva_xml_format_get_type (void) G_GNUC_CONST;

#define EVA_TYPE_XML_FORMAT (eva_xml_format_get_type ())
#define EVA_XML_FORMAT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_XML_FORMAT, EvaXmlFormat))
#define EVA_IS_XML_FORMAT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_XML_FORMAT))

struct _EvaXmlFormat
{
  GObject object;

  EvaGtypeLoader *type_loader;
};

/* Flag for properties that should be ignored by the serialize method.
 */
#define EVA_XML_FORMAT_PARAM_IGNORE (1 << G_PARAM_USER_SHIFT)

G_END_DECLS

#endif
