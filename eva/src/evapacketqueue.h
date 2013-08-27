#ifndef __EVA_PACKET_QUEUE_H_
#define __EVA_PACKET_QUEUE_H_

#include "evaio.h"
#include "evapacket.h"
#include "evasocketaddress.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _GskPacketQueue GskPacketQueue;
typedef struct _GskPacketQueueClass GskPacketQueueClass;
/* --- type macros --- */
GType eva_packet_queue_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_PACKET_QUEUE	           (eva_packet_queue_get_type ())
#define EVA_PACKET_QUEUE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_PACKET_QUEUE, GskPacketQueue))
#define EVA_PACKET_QUEUE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_PACKET_QUEUE, GskPacketQueueClass))
#define EVA_PACKET_QUEUE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_PACKET_QUEUE, GskPacketQueueClass))
#define EVA_IS_PACKET_QUEUE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_PACKET_QUEUE))
#define EVA_IS_PACKET_QUEUE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_PACKET_QUEUE))

/* --- structures --- */
struct _GskPacketQueueClass 
{
  GskIOClass io_class;
  gboolean   (*bind)  (GskPacketQueue    *queue,
		       GskSocketAddress  *addr,
		       GError           **error);
  GskPacket *(*read)  (GskPacketQueue    *queue,
		       gboolean           save_address,
		       GError           **error);
  gboolean   (*write) (GskPacketQueue    *queue,
		       GskPacket         *out,
		       GError           **error);

};
struct _GskPacketQueue 
{
  GskIO      io;
  guint      allow_address : 1;
  guint      allow_no_address : 1;
  guint      misses_packets : 1;
  GskSocketAddress *bound_address;
};

/* --- prototypes --- */
gboolean   eva_packet_queue_bind  (GskPacketQueue    *queue,
				   GskSocketAddress  *address,
		                   GError           **error);
GskPacket *eva_packet_queue_read  (GskPacketQueue    *queue,
				   gboolean           save_address,
		                   GError           **error);
gboolean   eva_packet_queue_write (GskPacketQueue    *queue,
		                   GskPacket         *out,
		                   GError           **error);
#define eva_packet_queue_get_allow_address(queue)       _eva_packet_queue_get(queue,allow_address)
#define eva_packet_queue_get_allow_no_address(queue)    _eva_packet_queue_get(queue,allow_no_address)
#define eva_packet_queue_get_misses_packets(queue)      _eva_packet_queue_get(queue,misses_packets)
#define eva_packet_queue_get_is_readable(queue)         _eva_packet_queue_get_io(queue,is_readable)
#define eva_packet_queue_get_is_writable(queue)         _eva_packet_queue_get_io(queue,is_writable)
#define eva_packet_queue_peek_bound_address(queue)      ((queue)->bound_address)


/*< protected >*/
#define eva_packet_queue_mark_allow_address(queue)      _eva_packet_queue_mark(queue,allow_address)
#define eva_packet_queue_mark_allow_no_address(queue)   _eva_packet_queue_mark(queue,allow_no_address)
#define eva_packet_queue_mark_misses_packets(queue)     _eva_packet_queue_mark(queue,misses_packets)
#define eva_packet_queue_mark_is_readable(queue)        _eva_packet_queue_mark_io(queue,is_readable)
#define eva_packet_queue_mark_is_writable(queue)        _eva_packet_queue_mark_io(queue,is_writable)
#define eva_packet_queue_clear_allow_address(queue)     _eva_packet_queue_clear(queue,allow_address)
#define eva_packet_queue_clear_allow_no_address(queue)  _eva_packet_queue_clear(queue,allow_no_address)
#define eva_packet_queue_clear_misses_packets(queue)    _eva_packet_queue_clear(queue,misses_packets)
#define eva_packet_queue_clear_is_readable(queue)       _eva_packet_queue_clear_io(queue,is_readable)
#define eva_packet_queue_clear_is_writable(queue)       _eva_packet_queue_clear_io(queue,is_writable)

/*< private >*/
#define _eva_packet_queue_get(queue, field)  ((queue)->field != 0)
#define _eva_packet_queue_mark(queue, field)  G_STMT_START{ (queue)->field = 1; }G_STMT_END
#define _eva_packet_queue_clear(queue, field)  G_STMT_START{ (queue)->field = 0; }G_STMT_END
#define _eva_packet_queue_get_io(queue, field)  _eva_packet_queue_get(queue,io.field)
#define _eva_packet_queue_mark_io(queue, field)  _eva_packet_queue_mark(queue,io.field)
#define _eva_packet_queue_clear_io(queue, field)  _eva_packet_queue_clear(queue,io.field)
void eva_packet_queue_set_bound_addresss (GskPacketQueue   *queue,
					  GskSocketAddress *address);

G_END_DECLS

#endif
