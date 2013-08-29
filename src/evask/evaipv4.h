#ifndef __EVA_IPV4_H
#define __EVA_IPV4_H

#include <glib.h>

G_BEGIN_DECLS

extern const guint8 eva_ipv4_ip_address_any[4];
extern const guint8 eva_ipv4_ip_address_loopback[4];

#define eva_ipv4_ip_address_localhost eva_ipv4_ip_address_loopback

/* hint: To obtain the broadcast address,
   use eva_network_interface_set_new(). */

/* Parse a numeric IP address. */
gboolean eva_ipv4_parse (const char *str, guint8 *ip_addr_out);

G_END_DECLS

#endif
