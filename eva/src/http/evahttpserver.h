#ifndef __EVA_HTTP_SERVER_H_
#define __EVA_HTTP_SERVER_H_

#include "evahttpheader.h"
#include "evahttprequest.h"
#include "evahttpresponse.h"
#include "../evastream.h"
#include "../evamainloop.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _GskHttpServer GskHttpServer;
typedef struct _GskHttpServerResponse GskHttpServerResponse;
typedef struct _GskHttpServerClass GskHttpServerClass;
/* --- type macros --- */
GType eva_http_server_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_HTTP_SERVER			(eva_http_server_get_type ())
#define EVA_HTTP_SERVER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_HTTP_SERVER, GskHttpServer))
#define EVA_HTTP_SERVER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_HTTP_SERVER, GskHttpServerClass))
#define EVA_HTTP_SERVER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_HTTP_SERVER, GskHttpServerClass))
#define EVA_IS_HTTP_SERVER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_HTTP_SERVER))
#define EVA_IS_HTTP_SERVER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_HTTP_SERVER))

typedef gboolean (*GskHttpServerTrap) (GskHttpServer *server,
				       gpointer       data);

/* --- structures --- */
struct _GskHttpServerClass 
{
  GskStreamClass stream_class;
  void         (*set_poll_request) (GskHttpServer *server,
				    gboolean       do_poll);
  void         (*shutdown_request) (GskHttpServer *server);
};
struct _GskHttpServer 
{
  GskStream      stream;
  GskHook        has_request_hook;
  GskBuffer      incoming;
  GskHttpServerResponse *first_response;
  GskHttpServerResponse *last_response;
  GskHttpServerResponse *trapped_response;
  guint read_poll : 1;
  guint write_poll : 1;
  guint got_close : 1;
  gint keepalive_idle_timeout_ms;       /* or -1 for no timeout */
  GskSource *keepalive_idle_timeout;
};

/* --- prototypes --- */
#define EVA_HTTP_SERVER_HOOK(client)	(&EVA_HTTP_SERVER (client)->has_request_hook)
#define eva_http_server_trap(server,func,shutdown,data,destroy)  	     \
	eva_hook_trap (EVA_HTTP_SERVER_HOOK (server), (GskHookFunc) func,    \
		       (GskHookFunc) shutdown, data, destroy)
#define eva_http_server_untrap(server)			     	             \
        eva_hook_untrap (EVA_HTTP_SERVER_HOOK (server))


GskHttpServer  *eva_http_server_new         (void);
gboolean        eva_http_server_get_request (GskHttpServer   *server,
					     GskHttpRequest **request_out,
					     GskStream      **post_data_out);
void            eva_http_server_respond     (GskHttpServer   *server,
					     GskHttpRequest  *request,
					     GskHttpResponse *response,
					     GskStream       *content);
void            eva_http_server_set_idle_timeout
                                            (GskHttpServer   *server,
                                             gint             millis);




G_END_DECLS

#endif
