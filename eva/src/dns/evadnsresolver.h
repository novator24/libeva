#ifndef __EVA_RESOLVER_H_
#define __EVA_RESOLVER_H_

#include "../evasocketaddress.h"
#include "evadns.h"

G_BEGIN_DECLS

GType eva_dns_resolver_get_type () G_GNUC_CONST;
#define EVA_TYPE_DNS_RESOLVER		(eva_dns_resolver_get_type ())
#define EVA_DNS_RESOLVER(o)		(G_TYPE_CHECK_INSTANCE_CAST(o, EVA_TYPE_DNS_RESOLVER, EvaDnsResolver))
#define EVA_DNS_RESOLVER_GET_IFACE(o)	(G_TYPE_INSTANCE_GET_INTERFACE((o), EVA_TYPE_DNS_RESOLVER, EvaDnsResolverIface))
#define EVA_IS_DNS_RESOLVER(o)		(G_TYPE_CHECK_INSTANCE_TYPE((o), EVA_TYPE_DNS_RESOLVER))

typedef struct _EvaDnsResolverHints EvaDnsResolverHints;
typedef struct _EvaDnsResolverIface EvaDnsResolverIface;
typedef struct _EvaDnsResolver EvaDnsResolver;
typedef struct _EvaDnsResolverTask EvaDnsResolverTask;

typedef void (*EvaDnsResolverLookupFunc)   (EvaSocketAddress   *address,
				            gpointer            func_data);
typedef void (*EvaDnsResolverRevLookupFunc)(const char         *name,
				            gpointer            func_data);
typedef void (*EvaDnsResolverResponseFunc) (GSList             *answers,
					    GSList             *authority,
					    GSList             *additional,
					    GSList             *neg_questions,
					    gpointer            func_data);
typedef void (*EvaDnsResolverFailFunc)     (GError             *error,
				            gpointer            func_data);

struct _EvaDnsResolverHints
{
  EvaSocketAddress       *address;
};

struct _EvaDnsResolverIface
{
  GTypeInterface base_iface;
  gpointer            (*resolve) (EvaDnsResolver               *resolver,
				  gboolean                      recursive,
				  GSList                       *dns_questions,
				  EvaDnsResolverResponseFunc    func,
				  EvaDnsResolverFailFunc        on_fail,
				  gpointer                      func_data,
				  GDestroyNotify                destroy,
				  EvaDnsResolverHints          *optional_hints);
  void                (*cancel)  (EvaDnsResolver               *resolver,
				  gpointer                      task);
};

EvaDnsResolverTask *eva_dns_resolver_resolve (EvaDnsResolver        *resolver,
					      gboolean               recursive,
				              GSList                *dns_questions,
				              EvaDnsResolverResponseFunc func,
				              EvaDnsResolverFailFunc on_fail,
				              gpointer               func_data,
                                              GDestroyNotify         destroy,
					      EvaDnsResolverHints   *hints);
void            eva_dns_resolver_cancel      (EvaDnsResolver        *resolver,
					      EvaDnsResolverTask    *task);

/* helper functions to use this interface to do simple DNS lookups */
/* TODO: ipv6 support */
EvaDnsResolverTask *eva_dns_resolver_lookup  (EvaDnsResolver        *resolver,
				              const char            *name,
				              EvaDnsResolverLookupFunc func,
				              EvaDnsResolverFailFunc on_fail,
				              gpointer               func_data,
				              GDestroyNotify         destroy);
EvaDnsResolverTask *eva_dns_resolver_rev_lookup 
                                             (EvaDnsResolver        *resolver,
				              const char            *name,
				              EvaDnsResolverRevLookupFunc func,
				              EvaDnsResolverFailFunc on_fail,
				              gpointer               func_data,
				              GDestroyNotify         destroy);

/* protected hack: add a name-resolver interface based on the dns-resolver interface */
void eva_dns_resolver_add_name_resolver_iface (GType type);
G_END_DECLS

#endif
