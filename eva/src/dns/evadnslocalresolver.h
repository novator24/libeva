#ifndef __EVA_DNS_LOCAL_RESOLVER_H_
#define __EVA_DNS_LOCAL_RESOLVER_H_

#include "evadnsrrcache.h"
#include "evadnsresolver.h"

G_BEGIN_DECLS

typedef struct _GskDnsLocalResolverClass GskDnsLocalResolverClass;
typedef struct _GskDnsLocalResolver GskDnsLocalResolver;

GType eva_dns_local_resolver_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_DNS_LOCAL_RESOLVER			(eva_dns_local_resolver_get_type ())
#define EVA_DNS_LOCAL_RESOLVER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_DNS_LOCAL_RESOLVER, GskDnsLocalResolver))
#define EVA_DNS_LOCAL_RESOLVER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_DNS_LOCAL_RESOLVER, GskDnsLocalResolverClass))
#define EVA_DNS_LOCAL_RESOLVER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_DNS_LOCAL_RESOLVER, GskDnsLocalResolverClass))
#define EVA_IS_DNS_LOCAL_RESOLVER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_DNS_LOCAL_RESOLVER))
#define EVA_IS_DNS_LOCAL_RESOLVER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_DNS_LOCAL_RESOLVER))


GskDnsResolver *eva_dns_local_resolver_new   (GskDnsRRCache      *rr_cache);

/* --- implementation details --- */
typedef enum
{
  EVA_DNS_LOCAL_NO_DATA,
  EVA_DNS_LOCAL_PARTIAL_DATA,
  EVA_DNS_LOCAL_NEGATIVE,
  EVA_DNS_LOCAL_SUCCESS
} GskDnsLocalResult;

GskDnsLocalResult eva_dns_local_resolver_answer(GskDnsRRCache      *rr_cache,
					        GskDnsQuestion     *question,
				                GskDnsMessage      *results);
G_END_DECLS

#endif
