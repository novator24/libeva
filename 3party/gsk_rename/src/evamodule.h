#ifndef __EVA_MODULE_H_
#define __EVA_MODULE_H_

#include <glib.h>
#include <gmodule.h>

G_BEGIN_DECLS

typedef struct _EvaCompileContext EvaCompileContext;
typedef struct _EvaModule EvaModule;

EvaCompileContext *eva_compile_context_new        (void);
void               eva_compile_context_add_cflags (EvaCompileContext *context,
                                                   const char        *flags);
void               eva_compile_context_add_ldflags(EvaCompileContext *context,
                                                   const char        *flags);
void               eva_compile_context_add_pkg    (EvaCompileContext *context,
                                                   const char        *pkg);
void               eva_compile_context_set_tmp_dir(EvaCompileContext *context,
                                                   const char        *tmp_dir);
void               eva_compile_context_set_gdb    (EvaCompileContext *context,
                                                   gboolean           support);
void               eva_compile_context_set_verbose(EvaCompileContext *context,
                                                   gboolean           support);
void               eva_compile_context_free       (EvaCompileContext *context);

/* a wrapper around GModule with ref-counting,
 * and the ability to delete itself. */

EvaModule *eva_module_compile (EvaCompileContext *context,
                               guint              n_sources,
                               char             **sources,
                               GModuleFlags       flags,
                               gboolean           delete_sources,
                               char             **program_output,
                               GError           **error);
EvaModule *eva_module_open    (const char        *filename,
                               GModuleFlags       flags,
                               GError           **error);

EvaModule *eva_module_ref     (EvaModule *module);
void       eva_module_unref   (EvaModule *module);
gpointer   eva_module_lookup  (EvaModule *module,
                               const char *symbol_name);



G_END_DECLS

#endif
