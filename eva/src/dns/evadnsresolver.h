#ifndef __EVA_RESOLVER_H_
#define __EVA_RESOLVER_H_

#include "../evasocketaddress.h"
#include "evadns.h"

G_BEGIN_DECLS

GType eva_dns_resolver_get_type () G_GNUC_CONST;
#define EVA_TYPE_DNS_RESOLVER		(eva_dns_resolver_get_type ())
#define EVA_DNS_RESOLVER(o)		(G_TYPE_CHECK_INSTANCE_CAST(o, EVA_TYPE_DNS_RESOLVER, GskDnsResolver))
#define EVA_DNS_RESOLVER_GET_IFACE(o)	(G_TYPE_INSTANCE_GET_INTERFACE((o), EVA_TYPE_DNS_RESOLVER, GskDnsResolverIface))
#define EVA_IS_DNS_RESOLVER(o)		(G_TYPE_CHECK_INSTANCE_TYPE((o), EVA_TYPE_DNS_RESOLVER))

typedef struct _GskDnsResolverHints GskDnsResolverHints;
typedef struct _GskDnsResolverIface GskDnsResolverIface;
typedef struct _GskDnsResolver GskDnsResolver;
typedef struct _GskDnsResolverTask GskDnsResolverTask;

typedef void (*GskDnsResolverLookupFunc)   (GskSocketAddress   *address,
				            gpointer            func_data);
typedef void (*GskDnsResolverRevLookupFunc)(const char         *name,
				            gpointer            func_data);
typedef void (*GskDnsResolverResponseFunc) (GSList             *answers,
					    GSList             *authority,
					    GSList             *additional,
					    GSList             *neg_questions,
					    gpointer            func_data);
typedef void (*GskDnsResolverFailFunc)     (GError             *error,
				            gpointer            func_data);

struct _GskDnsResolverHints
{
  GskSocketAddress       *address;
};

struct _GskDnsResolverIface
{
  GTypeInterface base_iface;
  gpointer            (*resolve) (GskDnsResolver               *resolver,
				  gboolean                      recursive,
				  GSList                       *dns_questions,
				  GskDnsResolverResponseFunc    func,
				  GskDnsResolverFailFunc        on_fail,
				  gpointer                      func_data,
				  GDestroyNotify                destroy,
				  GskDnsResolverHints          *optional_hints);
  void                (*cancel)  (GskDnsResolver               *resolver,
				  gpointer                      task);
};

GskDnsResolverTask *eva_dns_resolver_resolve (GskDnsResolver        *resolver,
					      gboolean               recursive,
				              GSList                *dns_questions,
				              GskDnsResolverResponseFunc func,
				              GskDnsResolverFailFunc on_fail,
				              gpointer               func_data,
                                              GDestroyNotify         destroy,
					      GskDnsResolverHints   *hints);
void            eva_dns_resolver_cancel      (GskDnsResolver        *resolver,
					      GskDnsResolverTask    *task);

/* helper functions to use this interface to do simple DNS lookups */
/* TODO: ipv6 support */
GskDnsResolverTask *eva_dns_resolver_lookup  (GskDnsResolver        *resolver,
				              const char            *name,
				              GskDnsResolverLookupFunc func,
				              GskDnsResolverFailFunc on_fail,
				              gpointer               func_data,
				              GDestroyNotify         destroy);
GskDnsResolverTask *eva_dns_resolver_rev_lookup 
                                             (GskDnsResolver        *resolver,
				              const char            *name,
				              GskDnsResolverRevLookupFunc func,
				              GskDnsResolverFailFunc on_fail,
				              gpointer               func_data,
				              GDestroyNotify         destroy);

/* protected hack: add a name-resolver interface based on the dns-resolver interface */
void eva_dns_resolver_add_name_resolver_iface (GType type);
G_END_DECLS

#endif
