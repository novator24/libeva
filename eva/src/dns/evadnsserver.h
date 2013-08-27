#ifndef __EVA_DNS_SERVER_H_
#define __EVA_DNS_SERVER_H_

#include "evadnsresolver.h"
#include "../evapacketqueue.h"

G_BEGIN_DECLS

typedef struct _EvaDnsServerClass EvaDnsServerClass;
typedef struct _EvaDnsServer EvaDnsServer;

GType eva_dns_server_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_DNS_SERVER			(eva_dns_server_get_type ())
#define EVA_DNS_SERVER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_DNS_SERVER, EvaDnsServer))
#define EVA_DNS_SERVER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_DNS_SERVER, EvaDnsServerClass))
#define EVA_DNS_SERVER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_DNS_SERVER, EvaDnsServerClass))
#define EVA_IS_DNS_SERVER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_DNS_SERVER))
#define EVA_IS_DNS_SERVER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_DNS_SERVER))

EvaDnsServer   *eva_dns_server_new           (EvaDnsResolver     *resolver,
					      EvaPacketQueue     *packet_queue);
EvaDnsResolver *eva_dns_server_peek_resolver (EvaDnsServer       *server);
void            eva_dns_server_set_resolver  (EvaDnsServer       *server,
					      EvaDnsResolver     *resolver);

G_END_DECLS

#endif
