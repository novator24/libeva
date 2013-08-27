#ifndef __EVA_NAME_RESOLVER_H_
#define __EVA_NAME_RESOLVER_H_

#include "evasocketaddress.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _GskNameResolver GskNameResolver;
typedef struct _GskNameResolverIface GskNameResolverIface;
typedef struct _GskNameResolverTask GskNameResolverTask;

/* --- type macros --- */
GType eva_name_resolver_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_NAME_RESOLVER              (eva_name_resolver_get_type ())
#define EVA_NAME_RESOLVER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_NAME_RESOLVER, GskNameResolver))
#define EVA_NAME_RESOLVER_GET_IFACE(obj)    (G_TYPE_INSTANCE_GET_INTERFACE ((obj), EVA_TYPE_NAME_RESOLVER, GskNameResolverIface))
#define EVA_IS_NAME_RESOLVER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_NAME_RESOLVER))

typedef void (*GskNameResolverSuccessFunc) (GskSocketAddress *address,
                                            gpointer          func_data);
typedef void (*GskNameResolverFailureFunc) (GError           *error,
                                            gpointer          func_data);

typedef enum
{
  EVA_NAME_RESOLVER_FAMILY_NONE,
  EVA_NAME_RESOLVER_FAMILY_IPV4
} GskNameResolverFamily;

/* --- structures --- */
struct _GskNameResolverIface
{
  GObjectClass object_class;
  gpointer (*start_resolve)  (GskNameResolver           *resolver,
			      GskNameResolverFamily      family,
                              const char                *name,
                              GskNameResolverSuccessFunc success,
                              GskNameResolverFailureFunc failure,
                              gpointer                   func_data,
                              GDestroyNotify             destroy);
  gboolean (*cancel_resolve) (GskNameResolver           *resolver,
                              gpointer                   handle);
};


/* --- prototypes --- */

/* handling families of names */
GskNameResolverFamily eva_name_resolver_family_get_by_name (const char *name);
const char           *eva_name_resolver_family_get_name(GskNameResolverFamily family);

/* easy, non-cancellable API */
void eva_name_resolver_lookup (GskNameResolverFamily       family,
                               const char                 *name,
                               GskNameResolverSuccessFunc  success,
                               GskNameResolverFailureFunc  failure,
                               gpointer                    func_data,
                               GDestroyNotify              destroy);

/* starting/cancelling name resolution */
GskNameResolverTask *
    eva_name_resolver_task_new        (GskNameResolverFamily       family,
				       const char                 *name,
                                       GskNameResolverSuccessFunc  success,
                                       GskNameResolverFailureFunc  failure,
                                       gpointer                    func_data,
                                       GDestroyNotify              destroy);
void eva_name_resolver_task_cancel (GskNameResolverTask *task);
void eva_name_resolver_task_ref (GskNameResolverTask *task);
void eva_name_resolver_task_unref (GskNameResolverTask *task);

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
GskNameResolverFamily eva_name_resolver_family_unique (const char *name);
void eva_name_resolver_add_family_name    (GskNameResolverFamily   family,
					   const char             *name);

typedef GskNameResolver *(*GskNameResolverFamilyHandler) (gpointer  data);
void eva_name_resolver_add_family_resolver (GskNameResolverFamily   family,
				            GskNameResolver        *resolver);
void eva_name_resolver_add_family_handler (GskNameResolverFamily    family,
					   GskNameResolverFamilyHandler handler,
					   gpointer                 data,
					   GDestroyNotify           destroy);

G_END_DECLS

#endif
