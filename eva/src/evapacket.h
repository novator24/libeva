#ifndef __EVA_PACKET_H_
#define __EVA_PACKET_H_

#include <glib.h>
#include "evasocketaddress.h"

G_BEGIN_DECLS

typedef struct _GskPacket GskPacket;

typedef void (*GskPacketDestroyFunc) (gpointer   destroy_data,
				      GskPacket *packet);


struct _GskPacket
{
  /*< public: readonly >*/
  gpointer data;
  guint len;
  GskSocketAddress *src_address;
  GskSocketAddress *dst_address;
  GskPacketDestroyFunc destroy;
  gpointer destroy_data;

  /*< private >*/
  guint ref_count;
};

GskPacket *eva_packet_new             (gpointer              data,
                                       guint                 length,
			               GskPacketDestroyFunc  destroy,
			               gpointer              destroy_data);
GskPacket *eva_packet_new_copy        (gconstpointer         data,
                                       guint                 length);
void       eva_packet_set_src_address (GskPacket            *packet,
				       GskSocketAddress     *address);
void       eva_packet_set_dst_address (GskPacket            *packet,
				       GskSocketAddress     *address);
void       eva_packet_unref           (GskPacket            *packet);
GskPacket *eva_packet_ref             (GskPacket            *packet);

G_END_DECLS

#endif
