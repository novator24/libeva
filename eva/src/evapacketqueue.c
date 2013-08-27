#include "evapacketqueue.h"
#include "evamacros.h"

static GObjectClass *parent_class = NULL;

static void
eva_packet_queue_finalize (GObject *object)
{
  EvaPacketQueue *queue = EVA_PACKET_QUEUE (object);
  if (queue->bound_address != NULL)
    g_object_unref (queue->bound_address);
  (*parent_class->finalize) (object);
}

/* --- functions --- */
static void
eva_packet_queue_init (EvaPacketQueue *packet_queue)
{
}
static void
eva_packet_queue_class_init (EvaPacketQueueClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  parent_class = g_type_class_peek_parent (class);
  object_class->finalize = eva_packet_queue_finalize;
}

GType eva_packet_queue_get_type()
{
  static GType packet_queue_type = 0;
  if (!packet_queue_type)
    {
      static const GTypeInfo packet_queue_info =
      {
	sizeof(EvaPacketQueueClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_packet_queue_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EvaPacketQueue),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_packet_queue_init,
	NULL		/* value_table */
      };
      packet_queue_type = g_type_register_static (EVA_TYPE_IO,
                                                  "EvaPacketQueue",
						  &packet_queue_info, 
						  G_TYPE_FLAG_ABSTRACT);
    }
  return packet_queue_type;
}

/* --- public methods --- */

/**
 * eva_packet_queue_bind:
 * @queue: the packet queue to bind to a particular address.
 * @address: the address to receive packets from.
 * @error: optional error return value.
 *
 * Bind a packet queue to receive on a particular address.
 *
 * returns: whether the bind call succeeded.
 */
gboolean
eva_packet_queue_bind   (EvaPacketQueue    *queue,
			 EvaSocketAddress  *address,
			 GError           **error)
{
  EvaPacketQueueClass *class = EVA_PACKET_QUEUE_GET_CLASS (queue);
  if (class->bind == NULL)
    {
      g_set_error (error, EVA_G_ERROR_DOMAIN,
                   EVA_ERROR_BIND_UNAVAILABLE,
		   _("no bind method for %s"),
		   g_type_name (G_OBJECT_TYPE (queue)));
      return FALSE;
    }
  if (! (*class->bind) (queue, address, error))
    {
      if (error && !*error)
	g_set_error (error, EVA_G_ERROR_DOMAIN,
		     EVA_ERROR_BIND_FAILED,
		     _("bind on %s failed, no specific error message"),
		     g_type_name (G_OBJECT_TYPE (queue)));
      return FALSE;
    }
  eva_packet_queue_set_bound_addresss (queue, address);
  return TRUE;
}

/**
 * eva_packet_queue_read:
 * @queue: to try to read a packet from
 * @save_address: whether to create a remote address indication.
 * @error: optional error return value.
 *
 * Read a packet from the queue, optionally tracking whence it came.
 *
 * returns: a new packet, or NULL on error or if no packet was available.
 */
EvaPacket *
eva_packet_queue_read   (EvaPacketQueue    *queue,
			 gboolean           save_address,
			 GError           **error)
{
  EvaPacketQueueClass *class = EVA_PACKET_QUEUE_GET_CLASS (queue);
  g_return_val_if_fail (!save_address || queue->allow_address, NULL);
  return (*class->read) (queue, save_address, error);
}

/**
 * eva_packet_queue_write:
 * @queue: to try to write a packet to
 * @out: outgoing packet.
 * @error: optional error return value.
 *
 * Try and write a packet to the queue.
 * If the packet has no address, then the packet-queue must allow_no_address.
 * If the packet has an address, then the packet-queue must allow_address.
 *
 * returns: whether the write succeeded.
 */
gboolean
eva_packet_queue_write  (EvaPacketQueue    *queue,
			 EvaPacket         *out,
			 GError           **error)
{
  EvaPacketQueueClass *class = EVA_PACKET_QUEUE_GET_CLASS (queue);
  g_return_val_if_fail (out->dst_address == NULL || queue->allow_address, FALSE);
  g_return_val_if_fail (out->dst_address != NULL || queue->allow_no_address, FALSE);
  return (*class->write) (queue, out, error);
}

/* --- protected methods --- */
/**
 * eva_packet_queue_set_bound_addresss:
 * @queue: queue to mark
 * @address: address to which the queue has been bound.
 *
 * Set the bound_address member of the packet-queue safely.
 *
 * This should only be called by implementations
 * which automatically bind to certain addresses.
 */
void
eva_packet_queue_set_bound_addresss (EvaPacketQueue   *queue,
				     EvaSocketAddress *address)
{
  EvaSocketAddress *old_address = queue->bound_address;
  queue->bound_address = address;
  if (address != NULL)
    g_object_ref (address);
  if (old_address != NULL)
    g_object_unref (old_address);
}
