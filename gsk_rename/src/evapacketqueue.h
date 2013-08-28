#ifndef __EVA_PACKET_QUEUE_H_
#define __EVA_PACKET_QUEUE_H_

#include "evaio.h"
#include "evapacket.h"
#include "evasocketaddress.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _EvaPacketQueue EvaPacketQueue;
typedef struct _EvaPacketQueueClass EvaPacketQueueClass;
/* --- type macros --- */
GType eva_packet_queue_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_PACKET_QUEUE	           (eva_packet_queue_get_type ())
#define EVA_PACKET_QUEUE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_PACKET_QUEUE, EvaPacketQueue))
#define EVA_PACKET_QUEUE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_PACKET_QUEUE, EvaPacketQueueClass))
#define EVA_PACKET_QUEUE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_PACKET_QUEUE, EvaPacketQueueClass))
#define EVA_IS_PACKET_QUEUE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_PACKET_QUEUE))
#define EVA_IS_PACKET_QUEUE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_PACKET_QUEUE))

/* --- structures --- */
struct _EvaPacketQueueClass 
{
  EvaIOClass io_class;
  gboolean   (*bind)  (EvaPacketQueue    *queue,
		       EvaSocketAddress  *addr,
		       GError           **error);
  EvaPacket *(*read)  (EvaPacketQueue    *queue,
		       gboolean           save_address,
		       GError           **error);
  gboolean   (*write) (EvaPacketQueue    *queue,
		       EvaPacket         *out,
		       GError           **error);

};
struct _EvaPacketQueue 
{
  EvaIO      io;
  guint      allow_address : 1;
  guint      allow_no_address : 1;
  guint      misses_packets : 1;
  EvaSocketAddress *bound_address;
};

/* --- prototypes --- */
gboolean   eva_packet_queue_bind  (EvaPacketQueue    *queue,
				   EvaSocketAddress  *address,
		                   GError           **error);
EvaPacket *eva_packet_queue_read  (EvaPacketQueue    *queue,
				   gboolean           save_address,
		                   GError           **error);
gboolean   eva_packet_queue_write (EvaPacketQueue    *queue,
		                   EvaPacket         *out,
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
void eva_packet_queue_set_bound_addresss (EvaPacketQueue   *queue,
					  EvaSocketAddress *address);

G_END_DECLS

#endif
