#ifndef __EVA_NAME_RESOLVER_H_
#define __EVA_NAME_RESOLVER_H_

#include "evasocketaddress.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _EvaNameResolver EvaNameResolver;
typedef struct _EvaNameResolverIface EvaNameResolverIface;
typedef struct _EvaNameResolverTask EvaNameResolverTask;

/* --- type macros --- */
GType eva_name_resolver_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_NAME_RESOLVER              (eva_name_resolver_get_type ())
#define EVA_NAME_RESOLVER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_NAME_RESOLVER, EvaNameResolver))
#define EVA_NAME_RESOLVER_GET_IFACE(obj)    (G_TYPE_INSTANCE_GET_INTERFACE ((obj), EVA_TYPE_NAME_RESOLVER, EvaNameResolverIface))
#define EVA_IS_NAME_RESOLVER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_NAME_RESOLVER))

typedef void (*EvaNameResolverSuccessFunc) (EvaSocketAddress *address,
                                            gpointer          func_data);
typedef void (*EvaNameResolverFailureFunc) (GError           *error,
                                            gpointer          func_data);

typedef enum
{
  EVA_NAME_RESOLVER_FAMILY_NONE,
  EVA_NAME_RESOLVER_FAMILY_IPV4
} EvaNameResolverFamily;

/* --- structures --- */
struct _EvaNameResolverIface
{
  GObjectClass object_class;
  gpointer (*start_resolve)  (EvaNameResolver           *resolver,
			      EvaNameResolverFamily      family,
                              const char                *name,
                              EvaNameResolverSuccessFunc success,
                              EvaNameResolverFailureFunc failure,
                              gpointer                   func_data,
                              GDestroyNotify             destroy);
  gboolean (*cancel_resolve) (EvaNameResolver           *resolver,
                              gpointer                   handle);
};


/* --- prototypes --- */

/* handling families of names */
EvaNameResolverFamily eva_name_resolver_family_get_by_name (const char *name);
const char           *eva_name_resolver_family_get_name(EvaNameResolverFamily family);

/* easy, non-cancellable API */
void eva_name_resolver_lookup (EvaNameResolverFamily       family,
                               const char                 *name,
                               EvaNameResolverSuccessFunc  success,
                               EvaNameResolverFailureFunc  failure,
                               gpointer                    func_data,
                               GDestroyNotify              destroy);

/* starting/cancelling name resolution */
EvaNameResolverTask *
    eva_name_resolver_task_new        (EvaNameResolverFamily       family,
				       const char                 *name,
                                       EvaNameResolverSuccessFunc  success,
                                       EvaNameResolverFailureFunc  failure,
                                       gpointer                    func_data,
                                       GDestroyNotify              destroy);
void eva_name_resolver_task_cancel (EvaNameResolverTask *task);
void eva_name_resolver_task_ref (EvaNameResolverTask *task);
void eva_name_resolver_task_unref (EvaNameResolverTask *task);

#ifndef EVA_DISABLE_DEPRECATED
#define eva_name_resolve eva_name_resolver_task_new
#endif

#define eva_name_resolver_lookup(family, name, success, failure, func_data, destroy) \
  eva_name_resolver_task_unref(eva_name_resolver_task_new(family, name, success, failure, func_data, destroy))

/* tweaking the default name resolver */
void eva_name_resolver_set_dns_cache_size (guint64 max_bytes,
                                           guint   max_records);
void eva_name_resolver_set_dns_roundrobin (gboolean do_roundrobin);


/*< protected: for use by implementors of new families only >*/
EvaNameResolverFamily eva_name_resolver_family_unique (const char *name);
void eva_name_resolver_add_family_name    (EvaNameResolverFamily   family,
					   const char             *name);

typedef EvaNameResolver *(*EvaNameResolverFamilyHandler) (gpointer  data);
void eva_name_resolver_add_family_resolver (EvaNameResolverFamily   family,
				            EvaNameResolver        *resolver);
void eva_name_resolver_add_family_handler (EvaNameResolverFamily    family,
					   EvaNameResolverFamilyHandler handler,
					   gpointer                 data,
					   GDestroyNotify           destroy);

G_END_DECLS

#endif
