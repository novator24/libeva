#ifndef __EVA_DNS_CLIENT_H_
#define __EVA_DNS_CLIENT_H_

#include "evadns.h"
#include "evadnsrrcache.h"
#include "../evapacketqueue.h"
#include <glib-object.h>

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _GskDnsClient GskDnsClient;
typedef struct _GskDnsClientClass GskDnsClientClass;

/* --- type macros --- */
GType eva_dns_client_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_DNS_CLIENT			(eva_dns_client_get_type ())
#define EVA_DNS_CLIENT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_DNS_CLIENT, GskDnsClient))
#define EVA_DNS_CLIENT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_DNS_CLIENT, GskDnsClientClass))
#define EVA_DNS_CLIENT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_DNS_CLIENT, GskDnsClientClass))
#define EVA_IS_DNS_CLIENT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_DNS_CLIENT))
#define EVA_IS_DNS_CLIENT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_DNS_CLIENT))

/* --- prototypes --- */
typedef enum
{
  EVA_DNS_CLIENT_STUB_RESOLVER = (1<<0)
} GskDnsClientFlags;

GskDnsClient   *eva_dns_client_new           (GskPacketQueue     *packet_queue,
					      GskDnsRRCache      *rr_cache,
					      GskDnsClientFlags   flags);
void            eva_dns_client_add_searchpath(GskDnsClient       *client,
					      const char         *searchpath);
void            eva_dns_client_add_ns        (GskDnsClient       *client,
					      GskSocketAddressIpv4 *address);
void            eva_dns_client_set_cache     (GskDnsClient       *client,
					      GskDnsRRCache      *rr_cache);
void            eva_dns_client_set_flags     (GskDnsClient       *client,
					      GskDnsClientFlags   flags);
GskDnsClientFlags eva_dns_client_get_flags   (GskDnsClient       *client);
gboolean    eva_dns_client_parse_system_files(GskDnsClient       *client);


G_END_DECLS

#endif
