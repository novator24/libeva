#ifndef __EVA_STREAM_LISTENER_SOCKET_H_
#define __EVA_STREAM_LISTENER_SOCKET_H_

#include "evastreamlistener.h"
#include "evasocketaddress.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _GskStreamListenerSocket GskStreamListenerSocket;
typedef struct _GskStreamListenerSocketClass GskStreamListenerSocketClass;
/* --- type macros --- */
GType eva_stream_listener_socket_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_STREAM_LISTENER_SOCKET			(eva_stream_listener_socket_get_type ())
#define EVA_STREAM_LISTENER_SOCKET(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_STREAM_LISTENER_SOCKET, GskStreamListenerSocket))
#define EVA_STREAM_LISTENER_SOCKET_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_STREAM_LISTENER_SOCKET, GskStreamListenerSocketClass))
#define EVA_STREAM_LISTENER_SOCKET_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_STREAM_LISTENER_SOCKET, GskStreamListenerSocketClass))
#define EVA_IS_STREAM_LISTENER_SOCKET(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_STREAM_LISTENER_SOCKET))
#define EVA_IS_STREAM_LISTENER_SOCKET_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_STREAM_LISTENER_SOCKET))

#define EVA_STREAM_LISTENER_SOCKET_USE_GLIB_MAIN_LOOP	0

#if !EVA_STREAM_LISTENER_SOCKET_USE_GLIB_MAIN_LOOP
#include "evamainloop.h"
#endif

/* --- structures --- */
struct _GskStreamListenerSocketClass 
{
  GskStreamListenerClass stream_listener_class;
};
struct _GskStreamListenerSocket 
{
  GskStreamListener      stream_listener;
  gint                   fd;
#if EVA_STREAM_LISTENER_SOCKET_USE_GLIB_MAIN_LOOP
  GPollFD                poll_fd;
  GSource               *source;
#else
  GskSource             *source;
#endif 
  GskSocketAddress      *listening_address;
  gboolean               may_reuse_address;
  gboolean               unlink_when_done;      /* only available if listening_address is 'local' */
};

/* --- prototypes --- */
GskStreamListener *
eva_stream_listener_socket_new_bind     (GskSocketAddress *address,
					 GError          **error);

GskStreamListener *
eva_stream_listener_socket_new_from_fd (int      fd,
                                        GError **error);


/* --- tenative (not recommended at this point) --- */
typedef enum
{
  EVA_STREAM_LISTENER_SOCKET_DONT_REUSE_ADDRESS = (1<<0),
  EVA_STREAM_LISTENER_SOCKET_UNLINK_WHEN_DONE = (1<<1)
} GskStreamListenerSocketFlags;
GType eva_stream_listener_socket_flags_get_type (void) G_GNUC_CONST;

GskStreamListener *
eva_stream_listener_socket_new_bind_full(GskSocketAddress *address,
					 GskStreamListenerSocketFlags flags,
					 GError          **error);
void    eva_stream_listener_socket_set_backlog (GskStreamListenerSocket *lis,
						guint             backlog);


/*< private >*/
void _eva_socket_address_local_maybe_delete_stale_socket (GskSocketAddress *local_socket);
G_END_DECLS

#endif
