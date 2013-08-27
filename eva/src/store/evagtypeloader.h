#ifndef __EVA_GTYPE_LOADER_H_
#define __EVA_GTYPE_LOADER_H_

/*
 *
 * EvaGtypeLoader
 *
 */

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _EvaGtypeLoader EvaGtypeLoader;

/* Callback to load a type by name. */
typedef GType    (*EvaLoadTypeFunc) (const char  *type_name,
				     gpointer     user_data,
				     GError     **error);

/* Callback to test whether a EvaGtypeLoader should permit
 * instantiation of type.
 */
typedef gboolean (*EvaTestTypeFunc) (GType        type,
				     gpointer     user_data);


EvaGtypeLoader * eva_gtype_loader_new        (void);

void             eva_gtype_loader_ref        (EvaGtypeLoader  *loader);
void             eva_gtype_loader_unref      (EvaGtypeLoader  *loader);

/* Permit certain types to be instantiated with this EvaGtypeLoader. */
void             eva_gtype_loader_add_type   (EvaGtypeLoader  *loader,
					      GType            type);
void             eva_gtype_loader_add_test   (EvaGtypeLoader  *loader,
					      EvaTestTypeFunc  test_func,
					      gpointer         test_data,
					      GDestroyNotify   test_destroy);

/* Check whether loader permits instantiation of type. */
gboolean         eva_gtype_loader_test_type  (EvaGtypeLoader  *loader,
					      GType            type);

/* Specify a mechanism to load a new type. */
void             eva_gtype_loader_set_loader (EvaGtypeLoader  *loader,
					      EvaLoadTypeFunc  load_func,
					      gpointer         load_data,
					      GDestroyNotify   load_destroy);

GType            eva_gtype_loader_load_type  (EvaGtypeLoader  *loader,
					      const char      *type_name,
					      GError         **error);

/* Get a shared default type loader. */
EvaGtypeLoader * eva_gtype_loader_default    (void);

/* A EvaLoadTypeFunc-function which scans the current binary for
 * types by named _get_type() function.
 */
GType            eva_load_type_introspective (const char      *type_name,
					      gpointer         unused,
					      GError         **error);
G_END_DECLS

#endif
