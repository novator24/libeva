#ifndef __EVA_DNS_LOCAL_RESOLVER_H_
#define __EVA_DNS_LOCAL_RESOLVER_H_

#include "evadnsrrcache.h"
#include "evadnsresolver.h"

G_BEGIN_DECLS

typedef struct _EvaDnsLocalResolverClass EvaDnsLocalResolverClass;
typedef struct _EvaDnsLocalResolver EvaDnsLocalResolver;

GType eva_dns_local_resolver_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_DNS_LOCAL_RESOLVER			(eva_dns_local_resolver_get_type ())
#define EVA_DNS_LOCAL_RESOLVER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_DNS_LOCAL_RESOLVER, EvaDnsLocalResolver))
#define EVA_DNS_LOCAL_RESOLVER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_DNS_LOCAL_RESOLVER, EvaDnsLocalResolverClass))
#define EVA_DNS_LOCAL_RESOLVER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_DNS_LOCAL_RESOLVER, EvaDnsLocalResolverClass))
#define EVA_IS_DNS_LOCAL_RESOLVER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_DNS_LOCAL_RESOLVER))
#define EVA_IS_DNS_LOCAL_RESOLVER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_DNS_LOCAL_RESOLVER))


EvaDnsResolver *eva_dns_local_resolver_new   (EvaDnsRRCache      *rr_cache);

/* --- implementation details --- */
typedef enum
{
  EVA_DNS_LOCAL_NO_DATA,
  EVA_DNS_LOCAL_PARTIAL_DATA,
  EVA_DNS_LOCAL_NEGATIVE,
  EVA_DNS_LOCAL_SUCCESS
} EvaDnsLocalResult;

EvaDnsLocalResult eva_dns_local_resolver_answer(EvaDnsRRCache      *rr_cache,
					        EvaDnsQuestion     *question,
				                EvaDnsMessage      *results);
G_END_DECLS

#endif
