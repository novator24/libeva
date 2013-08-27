#ifndef __EVA_HTTP_SERVER_H_
#define __EVA_HTTP_SERVER_H_

#include "evahttpheader.h"
#include "evahttprequest.h"
#include "evahttpresponse.h"
#include "../evastream.h"
#include "../evamainloop.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _EvaHttpServer EvaHttpServer;
typedef struct _EvaHttpServerResponse EvaHttpServerResponse;
typedef struct _EvaHttpServerClass EvaHttpServerClass;
/* --- type macros --- */
GType eva_http_server_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_HTTP_SERVER			(eva_http_server_get_type ())
#define EVA_HTTP_SERVER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_HTTP_SERVER, EvaHttpServer))
#define EVA_HTTP_SERVER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_HTTP_SERVER, EvaHttpServerClass))
#define EVA_HTTP_SERVER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_HTTP_SERVER, EvaHttpServerClass))
#define EVA_IS_HTTP_SERVER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_HTTP_SERVER))
#define EVA_IS_HTTP_SERVER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_HTTP_SERVER))

typedef gboolean (*EvaHttpServerTrap) (EvaHttpServer *server,
				       gpointer       data);

/* --- structures --- */
struct _EvaHttpServerClass 
{
  EvaStreamClass stream_class;
  void         (*set_poll_request) (EvaHttpServer *server,
				    gboolean       do_poll);
  void         (*shutdown_request) (EvaHttpServer *server);
};
struct _EvaHttpServer 
{
  EvaStream      stream;
  EvaHook        has_request_hook;
  EvaBuffer      incoming;
  EvaHttpServerResponse *first_response;
  EvaHttpServerResponse *last_response;
  EvaHttpServerResponse *trapped_response;
  guint read_poll : 1;
  guint write_poll : 1;
  guint got_close : 1;
  gint keepalive_idle_timeout_ms;       /* or -1 for no timeout */
  EvaSource *keepalive_idle_timeout;
};

/* --- prototypes --- */
#define EVA_HTTP_SERVER_HOOK(client)	(&EVA_HTTP_SERVER (client)->has_request_hook)
#define eva_http_server_trap(server,func,shutdown,data,destroy)  	     \
	eva_hook_trap (EVA_HTTP_SERVER_HOOK (server), (EvaHookFunc) func,    \
		       (EvaHookFunc) shutdown, data, destroy)
#define eva_http_server_untrap(server)			     	             \
        eva_hook_untrap (EVA_HTTP_SERVER_HOOK (server))


EvaHttpServer  *eva_http_server_new         (void);
gboolean        eva_http_server_get_request (EvaHttpServer   *server,
					     EvaHttpRequest **request_out,
					     EvaStream      **post_data_out);
void            eva_http_server_respond     (EvaHttpServer   *server,
					     EvaHttpRequest  *request,
					     EvaHttpResponse *response,
					     EvaStream       *content);
void            eva_http_server_set_idle_timeout
                                            (EvaHttpServer   *server,
                                             gint             millis);




G_END_DECLS

#endif
