#ifndef __EVA_HTTP_CLIENT_H_
#define __EVA_HTTP_CLIENT_H_

#include "evahttpheader.h"
#include "evahttprequest.h"
#include "evahttpresponse.h"
#include "../evastream.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _GskHttpClient GskHttpClient;
typedef struct _GskHttpClientClass GskHttpClientClass;
typedef struct _GskHttpClientRequest GskHttpClientRequest;
/* --- type macros --- */
GType eva_http_client_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_HTTP_CLIENT			(eva_http_client_get_type ())
#define EVA_HTTP_CLIENT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_HTTP_CLIENT, GskHttpClient))
#define EVA_HTTP_CLIENT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_HTTP_CLIENT, GskHttpClientClass))
#define EVA_HTTP_CLIENT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_HTTP_CLIENT, GskHttpClientClass))
#define EVA_IS_HTTP_CLIENT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_HTTP_CLIENT))
#define EVA_IS_HTTP_CLIENT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_HTTP_CLIENT))

typedef void (*GskHttpClientResponse) (GskHttpRequest  *request,
				       GskHttpResponse *response,
				       GskStream       *input,
				       gpointer         hook_data);
typedef gboolean (*GskHttpClientTrap) (GskHttpClient   *client,
				       gpointer         data);

/* --- structures --- */
struct _GskHttpClientClass 
{
  GskStreamClass stream_class;
  void         (*set_poll_requestable) (GskHttpClient *client,
					gboolean       do_poll);
  void         (*shutdown_requestable) (GskHttpClient *client);
};
struct _GskHttpClient 
{
  /*< private >*/
  GskStream             stream;
  GskHook               requestable;
  GskBuffer             incoming_data;
  GskHttpClientRequest *first_request;
  GskHttpClientRequest *last_request;
  GskHttpClientRequest *outgoing_request;
};

#define EVA_HTTP_CLIENT_FAST_NOTIFY		(1<<0)
#define EVA_HTTP_CLIENT_DEFERRED_SHUTDOWN	(1<<1)
#define EVA_HTTP_CLIENT_REQUIRES_READ_SHUTDOWN	(1<<2)
#define EVA_HTTP_CLIENT_PROPAGATE_CONTENT_READ_SHUTDOWN (1<<3)

#define EVA_HTTP_CLIENT_HOOK(client)	(&EVA_HTTP_CLIENT (client)->requestable)
#define EVA_HTTP_CLIENT_IS_FAST(client)	((EVA_HTTP_CLIENT_HOOK (client)->user_flags & EVA_HTTP_CLIENT_FAST_NOTIFY) == EVA_HTTP_CLIENT_FAST_NOTIFY)
#define eva_http_client_trap_requestable(client,func,shutdown,data,destroy)  \
	eva_hook_trap (EVA_HTTP_CLIENT_HOOK (client), (GskHookFunc) func,    \
		       (GskHookFunc) shutdown, data, destroy)
#define eva_http_client_untrap_requestable(client)			     \
        eva_hook_untrap (EVA_HTTP_CLIENT_HOOK (client))
#define eva_http_client_is_requestable(client)				     \
        EVA_HOOK_TEST_IS_AVAILABLE (EVA_HTTP_CLIENT_HOOK (client))

/* --- prototypes --- */
GskHttpClient *eva_http_client_new (void);
void eva_http_client_notify_fast (GskHttpClient     *client,
				  gboolean           is_fast);
void eva_http_client_request (GskHttpClient         *client,
			      GskHttpRequest        *request,
			      GskStream             *post_data,
			      GskHttpClientResponse  handle_response,
			      gpointer               hook_data,
			      GDestroyNotify         hook_destroy);

void eva_http_client_shutdown_when_done (GskHttpClient *client);
void eva_http_client_propagate_content_read_shutdown (GskHttpClient *client);
G_END_DECLS


#endif
