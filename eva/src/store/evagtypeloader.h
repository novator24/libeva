#ifndef __EVA_GTYPE_LOADER_H_
#define __EVA_GTYPE_LOADER_H_

/*
 *
 * GskGtypeLoader
 *
 */

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _GskGtypeLoader GskGtypeLoader;

/* Callback to load a type by name. */
typedef GType    (*GskLoadTypeFunc) (const char  *type_name,
				     gpointer     user_data,
				     GError     **error);

/* Callback to test whether a GskGtypeLoader should permit
 * instantiation of type.
 */
typedef gboolean (*GskTestTypeFunc) (GType        type,
				     gpointer     user_data);


GskGtypeLoader * eva_gtype_loader_new        (void);

void             eva_gtype_loader_ref        (GskGtypeLoader  *loader);
void             eva_gtype_loader_unref      (GskGtypeLoader  *loader);

/* Permit certain types to be instantiated with this GskGtypeLoader. */
void             eva_gtype_loader_add_type   (GskGtypeLoader  *loader,
					      GType            type);
void             eva_gtype_loader_add_test   (GskGtypeLoader  *loader,
					      GskTestTypeFunc  test_func,
					      gpointer         test_data,
					      GDestroyNotify   test_destroy);

/* Check whether loader permits instantiation of type. */
gboolean         eva_gtype_loader_test_type  (GskGtypeLoader  *loader,
					      GType            type);

/* Specify a mechanism to load a new type. */
void             eva_gtype_loader_set_loader (GskGtypeLoader  *loader,
					      GskLoadTypeFunc  load_func,
					      gpointer         load_data,
					      GDestroyNotify   load_destroy);

GType            eva_gtype_loader_load_type  (GskGtypeLoader  *loader,
					      const char      *type_name,
					      GError         **error);

/* Get a shared default type loader. */
GskGtypeLoader * eva_gtype_loader_default    (void);

/* A GskLoadTypeFunc-function which scans the current binary for
 * types by named _get_type() function.
 */
GType            eva_load_type_introspective (const char      *type_name,
					      gpointer         unused,
					      GError         **error);
G_END_DECLS

#endif
