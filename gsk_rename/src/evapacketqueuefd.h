#ifndef __EVA_PACKET_QUEUE_FD_H_
#define __EVA_PACKET_QUEUE_FD_H_

#include "evapacketqueue.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _EvaPacketQueueFd EvaPacketQueueFd;
typedef struct _EvaPacketQueueFdClass EvaPacketQueueFdClass;
/* --- type macros --- */
GType eva_packet_queue_fd_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_PACKET_QUEUE_FD			(eva_packet_queue_fd_get_type ())
#define EVA_PACKET_QUEUE_FD(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_PACKET_QUEUE_FD, EvaPacketQueueFd))
#define EVA_PACKET_QUEUE_FD_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_PACKET_QUEUE_FD, EvaPacketQueueFdClass))
#define EVA_PACKET_QUEUE_FD_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_PACKET_QUEUE_FD, EvaPacketQueueFdClass))
#define EVA_IS_PACKET_QUEUE_FD(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_PACKET_QUEUE_FD))
#define EVA_IS_PACKET_QUEUE_FD_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_PACKET_QUEUE_FD))

#define EVA_PACKET_QUEUE_FD_USE_GLIB_MAIN_LOOP	0

#if !EVA_PACKET_QUEUE_FD_USE_GLIB_MAIN_LOOP
#include "evamainloop.h"
#endif

/* --- structures --- */
struct _EvaPacketQueueFdClass 
{
  EvaPacketQueueClass packet_queue_class;
};
struct _EvaPacketQueueFd 
{
  EvaPacketQueue      packet_queue;

  int fd;
  EvaSocketAddress *bound_address;
#if EVA_PACKET_QUEUE_FD_USE_GLIB_MAIN_LOOP
  GPollFD poll_fd;
  GSource *source;
#else
  EvaSource *source;
#endif
};

/* --- prototypes --- */
EvaPacketQueue *eva_packet_queue_fd_new           (int               fd);
EvaPacketQueue *eva_packet_queue_fd_new_by_family (int        addr_family,
						   GError   **error);
EvaPacketQueue *eva_packet_queue_fd_new_bound     (EvaSocketAddress *address,
						   GError   **error);
gboolean eva_packet_queue_fd_set_broadcast (EvaPacketQueueFd *packet_queue_fd,
					    gboolean          allow_broadcast,
					    GError          **error);

G_END_DECLS

#endif
