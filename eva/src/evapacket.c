#include "evapacket.h"
#include "evamacros.h"

EVA_DECLARE_POOL_ALLOCATORS(EvaPacket, eva_packet, 32)

/**
 * eva_packet_new:
 * @data: binary data in the packet
 * @length: length of binary data
 * @destroy: method to destroy the data.
 * @destroy_data: the argument to the @destroy method.
 *
 * Creates a new packet with the given data.
 * The packet's ref-count is 1; it will be destroyed
 * when it gets to 0.
 *
 * returns: a new EvaPacket
 */
EvaPacket *
eva_packet_new   (gpointer             data,
		  guint                length,
		  EvaPacketDestroyFunc destroy,
		  gpointer             destroy_data)
{
  EvaPacket *rv = eva_packet_alloc ();
  rv->data = data;
  rv->len = length;
  rv->destroy = destroy;
  rv->destroy_data = destroy_data;
  rv->ref_count = 1;
  rv->src_address = NULL;
  rv->dst_address = NULL;
  return rv;
}

/**
 * eva_packet_new_copy:
 * @data: binary data to be copied into the packet
 * @length: length of binary data
 *
 * Creates a new packet with a copy of the given data.
 * The packet's ref-count is 1; it will be destroyed
 * when it gets to 0.
 *
 * returns: a new EvaPacket
 */
EvaPacket *
eva_packet_new_copy   (gconstpointer        data,
		       guint                length)
{
  gpointer copy = g_memdup (data, length);
  return eva_packet_new (copy, length, (EvaPacketDestroyFunc) g_free, copy);
}

/**
 * eva_packet_unref:
 * @packet: a packet to remove a reference from.
 *
 * Remove a reference-count from the packet, deleting the packet 
 * if it gets to 0.
 */
void
eva_packet_unref (EvaPacket *packet)
{
  g_return_if_fail (packet->ref_count > 0);
  --(packet->ref_count);
  if (packet->ref_count == 0)
    {
      if (packet->destroy != NULL)
	(*packet->destroy) (packet->destroy_data, packet);
      if (packet->src_address != NULL)
	g_object_unref (packet->src_address);
      if (packet->dst_address != NULL)
	g_object_unref (packet->dst_address);
      eva_packet_free (packet);
    }
}

/**
 * eva_packet_ref:
 * @packet: a packet to add a reference to.
 *
 * Add a reference-count to the packet.
 *
 * returns: the @packet, for convenience.
 */
EvaPacket *
eva_packet_ref   (EvaPacket *packet)
{
  g_return_val_if_fail (packet->ref_count > 0, packet);
  ++(packet->ref_count);
  return packet;
}

/**
 * eva_packet_set_src_address:
 * @packet: a packet whose source address should be changed.
 * @address: the new address.
 *
 * Change the source address associated with a packet.
 * This should be the address of the interface
 * the packet was sent from.
 */
void
eva_packet_set_src_address (EvaPacket        *packet,
			    EvaSocketAddress *address)
{
  if (address != NULL)
    g_object_ref (address);
  if (packet->src_address != NULL)
    g_object_unref (packet->src_address);
  packet->src_address = address;
}

/**
 * eva_packet_set_dst_address:
 * @packet: a packet whose destination address should be changed.
 * @address: the new address.
 *
 * Change the destination address associated with a packet.
 * This should be the address of the interface
 * the packet was sent to.
 */
void
eva_packet_set_dst_address (EvaPacket        *packet,
			    EvaSocketAddress *address)
{
  if (address != NULL)
    g_object_ref (address);
  if (packet->dst_address != NULL)
    g_object_unref (packet->dst_address);
  packet->dst_address = address;
}
