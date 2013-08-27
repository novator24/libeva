#ifndef __EVA_MODULE_H_
#define __EVA_MODULE_H_

#include <glib.h>
#include <gmodule.h>

G_BEGIN_DECLS

typedef struct _GskCompileContext GskCompileContext;
typedef struct _GskModule GskModule;

GskCompileContext *eva_compile_context_new        (void);
void               eva_compile_context_add_cflags (GskCompileContext *context,
                                                   const char        *flags);
void               eva_compile_context_add_ldflags(GskCompileContext *context,
                                                   const char        *flags);
void               eva_compile_context_add_pkg    (GskCompileContext *context,
                                                   const char        *pkg);
void               eva_compile_context_set_tmp_dir(GskCompileContext *context,
                                                   const char        *tmp_dir);
void               eva_compile_context_set_gdb    (GskCompileContext *context,
                                                   gboolean           support);
void               eva_compile_context_set_verbose(GskCompileContext *context,
                                                   gboolean           support);
void               eva_compile_context_free       (GskCompileContext *context);

/* a wrapper around GModule with ref-counting,
 * and the ability to delete itself. */

GskModule *eva_module_compile (GskCompileContext *context,
                               guint              n_sources,
                               char             **sources,
                               GModuleFlags       flags,
                               gboolean           delete_sources,
                               char             **program_output,
                               GError           **error);
GskModule *eva_module_open    (const char        *filename,
                               GModuleFlags       flags,
                               GError           **error);

GskModule *eva_module_ref     (GskModule *module);
void       eva_module_unref   (GskModule *module);
gpointer   eva_module_lookup  (GskModule *module,
                               const char *symbol_name);



G_END_DECLS

#endif
