/* HTTP and HTTPS url schemes */

#ifndef __EVA_URL_TRANSFER_HTTP_H_
#define __EVA_URL_TRANSFER_HTTP_H_

#include "evaurltransfer.h"
#include "../evanameresolver.h"
#include "../http/evahttprequest.h"
#include "../http/evahttpresponse.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _GskUrlTransferHttp GskUrlTransferHttp;
typedef struct _GskUrlTransferHttpModifierNode GskUrlTransferHttpModifierNode;
typedef struct _GskUrlTransferHttpClass GskUrlTransferHttpClass;
/* --- type macros --- */
GType eva_url_transfer_http_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_URL_TRANSFER_HTTP			(eva_url_transfer_http_get_type ())
#define EVA_URL_TRANSFER_HTTP(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_URL_TRANSFER_HTTP, GskUrlTransferHttp))
#define EVA_URL_TRANSFER_HTTP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_URL_TRANSFER_HTTP, GskUrlTransferHttpClass))
#define EVA_URL_TRANSFER_HTTP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_URL_TRANSFER_HTTP, GskUrlTransferHttpClass))
#define EVA_IS_URL_TRANSFER_HTTP(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_URL_TRANSFER_HTTP))
#define EVA_IS_URL_TRANSFER_HTTP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_URL_TRANSFER_HTTP))

/* --- structures --- */
struct _GskUrlTransferHttpClass 
{
  GskUrlTransferClass base_class;
};

struct _GskUrlTransferHttp 
{
  GskUrlTransfer      base_instance;
  GskUrlTransferHttpModifierNode *first_modifier, *last_modifier;

  char *ssl_cert, *ssl_key, *ssl_password;

  /* state */
  GskNameResolverTask *name_lookup;
  GskStream *raw_transport;
  guint request_count;
  guint response_count;
  guint undestroyed_requests;

  gboolean is_proxy;
};
/* --- prototypes --- */

/* SSL configuration */
void eva_url_transfer_http_set_ssl_cert    (GskUrlTransferHttp *http,
                                            const char         *cert_fname);
void eva_url_transfer_http_set_ssl_key     (GskUrlTransferHttp *http,
                                            const char         *key_fname);
void eva_url_transfer_http_set_ssl_password(GskUrlTransferHttp *http,
                                            const char         *password);

/* HTTP Request configuration */
void eva_url_transfer_http_set_user_agent  (GskUrlTransferHttp *http,
                                            const char         *user_agent);
void eva_url_transfer_http_add_extra_header(GskUrlTransferHttp *http,
                                            const char         *key,
                                            const char         *value);



/* generic HTTP request configuration */
typedef void (*GskUrlTransferHttpRequestModifierFunc)(GskHttpRequest *request,
                                                      gpointer        mod_data);

void eva_url_transfer_http_add_modifier (GskUrlTransferHttp *http,
                                         GskUrlTransferHttpRequestModifierFunc modifier,
                                         gpointer data,
                                         GDestroyNotify destroy);


void eva_url_transfer_http_set_proxy_address  (GskUrlTransferHttp *http,
                                               GskSocketAddress   *proxy_address);

G_END_DECLS

#endif
