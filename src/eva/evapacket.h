#ifndef __EVA_PACKET_H_
#define __EVA_PACKET_H_

#include <glib.h>
#include "evasocketaddress.h"

G_BEGIN_DECLS

typedef struct _EvaPacket EvaPacket;

typedef void (*EvaPacketDestroyFunc) (gpointer   destroy_data,
				      EvaPacket *packet);


struct _EvaPacket
{
  /*< public: readonly >*/
  gpointer data;
  guint len;
  EvaSocketAddress *src_address;
  EvaSocketAddress *dst_address;
  EvaPacketDestroyFunc destroy;
  gpointer destroy_data;

  /*< private >*/
  guint ref_count;
};

EvaPacket *eva_packet_new             (gpointer              data,
                                       guint                 length,
			               EvaPacketDestroyFunc  destroy,
			               gpointer              destroy_data);
EvaPacket *eva_packet_new_copy        (gconstpointer         data,
                                       guint                 length);
void       eva_packet_set_src_address (EvaPacket            *packet,
				       EvaSocketAddress     *address);
void       eva_packet_set_dst_address (EvaPacket            *packet,
				       EvaSocketAddress     *address);
void       eva_packet_unref           (EvaPacket            *packet);
EvaPacket *eva_packet_ref             (EvaPacket            *packet);

G_END_DECLS

#endif
