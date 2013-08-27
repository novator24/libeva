#ifndef __EVA_DNS_SERVER_H_
#define __EVA_DNS_SERVER_H_

#include "evadnsresolver.h"
#include "../evapacketqueue.h"

G_BEGIN_DECLS

typedef struct _GskDnsServerClass GskDnsServerClass;
typedef struct _GskDnsServer GskDnsServer;

GType eva_dns_server_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_DNS_SERVER			(eva_dns_server_get_type ())
#define EVA_DNS_SERVER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_DNS_SERVER, GskDnsServer))
#define EVA_DNS_SERVER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_DNS_SERVER, GskDnsServerClass))
#define EVA_DNS_SERVER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_DNS_SERVER, GskDnsServerClass))
#define EVA_IS_DNS_SERVER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_DNS_SERVER))
#define EVA_IS_DNS_SERVER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_DNS_SERVER))

GskDnsServer   *eva_dns_server_new           (GskDnsResolver     *resolver,
					      GskPacketQueue     *packet_queue);
GskDnsResolver *eva_dns_server_peek_resolver (GskDnsServer       *server);
void            eva_dns_server_set_resolver  (GskDnsServer       *server,
					      GskDnsResolver     *resolver);

G_END_DECLS

#endif
