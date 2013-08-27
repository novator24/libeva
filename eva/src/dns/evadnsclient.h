#ifndef __EVA_DNS_CLIENT_H_
#define __EVA_DNS_CLIENT_H_

#include "evadns.h"
#include "evadnsrrcache.h"
#include "../evapacketqueue.h"
#include <glib-object.h>

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _EvaDnsClient EvaDnsClient;
typedef struct _EvaDnsClientClass EvaDnsClientClass;

/* --- type macros --- */
GType eva_dns_client_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_DNS_CLIENT			(eva_dns_client_get_type ())
#define EVA_DNS_CLIENT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_DNS_CLIENT, EvaDnsClient))
#define EVA_DNS_CLIENT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_DNS_CLIENT, EvaDnsClientClass))
#define EVA_DNS_CLIENT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_DNS_CLIENT, EvaDnsClientClass))
#define EVA_IS_DNS_CLIENT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_DNS_CLIENT))
#define EVA_IS_DNS_CLIENT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_DNS_CLIENT))

/* --- prototypes --- */
typedef enum
{
  EVA_DNS_CLIENT_STUB_RESOLVER = (1<<0)
} EvaDnsClientFlags;

EvaDnsClient   *eva_dns_client_new           (EvaPacketQueue     *packet_queue,
					      EvaDnsRRCache      *rr_cache,
					      EvaDnsClientFlags   flags);
void            eva_dns_client_add_searchpath(EvaDnsClient       *client,
					      const char         *searchpath);
void            eva_dns_client_add_ns        (EvaDnsClient       *client,
					      EvaSocketAddressIpv4 *address);
void            eva_dns_client_set_cache     (EvaDnsClient       *client,
					      EvaDnsRRCache      *rr_cache);
void            eva_dns_client_set_flags     (EvaDnsClient       *client,
					      EvaDnsClientFlags   flags);
EvaDnsClientFlags eva_dns_client_get_flags   (EvaDnsClient       *client);
gboolean    eva_dns_client_parse_system_files(EvaDnsClient       *client);


G_END_DECLS

#endif
