#ifndef __EVA_XML_VALUE_READER_H_
#define __EVA_XML_VALUE_READER_H_

/*
 *
 * GskXmlValueReader -- streaming GValue instantiation from XML.
 *
 */

#include <glib-object.h>
#include "evagtypeloader.h"

G_BEGIN_DECLS

typedef struct _GskXmlValueReader GskXmlValueReader;

/* Callback to asynchronously return a GValue. */
typedef void (*GskXmlValueFunc) (const GValue *value, gpointer user_data);

GskXmlValueReader *
	 eva_xml_value_reader_new       (GskGtypeLoader    *type_loader,
					 GType              output_type,
					 GskXmlValueFunc    value_func,
					 gpointer           value_func_data,
					 GDestroyNotify     value_func_destroy);

void     eva_xml_value_reader_free      (GskXmlValueReader *reader);

gboolean eva_xml_value_reader_input     (GskXmlValueReader *reader,
					 const char        *input,
					 guint              len,
					 GError           **error);

void     eva_xml_value_reader_set_pos   (GskXmlValueReader *reader,
					 const char        *filename,
					 gint               lineno,
					 gint               charno);

gboolean eva_xml_value_reader_had_error (GskXmlValueReader *reader);


/* Load an object from an XML file, for program configuration, etc. */

GObject * eva_load_object_from_xml_file (const char      *path,
					 GskGtypeLoader  *type_loader,
					 GType            object_type,
					 GError         **error);

G_END_DECLS

#endif
