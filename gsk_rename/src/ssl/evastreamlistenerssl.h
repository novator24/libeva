#ifndef __EVA_STREAM_LISTENER_SSL_H_
#define __EVA_STREAM_LISTENER_SSL_H_

#include "../evastreamlistener.h"
#include "evastreamssl.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _EvaStreamListenerSsl EvaStreamListenerSsl;
typedef struct _EvaStreamListenerSslClass EvaStreamListenerSslClass;
/* --- type macros --- */
GType eva_stream_listener_ssl_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_STREAM_LISTENER_SSL			(eva_stream_listener_ssl_get_type ())
#define EVA_STREAM_LISTENER_SSL(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_STREAM_LISTENER_SSL, EvaStreamListenerSsl))
#define EVA_STREAM_LISTENER_SSL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_STREAM_LISTENER_SSL, EvaStreamListenerSslClass))
#define EVA_STREAM_LISTENER_SSL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_STREAM_LISTENER_SSL, EvaStreamListenerSslClass))
#define EVA_IS_STREAM_LISTENER_SSL(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_STREAM_LISTENER_SSL))
#define EVA_IS_STREAM_LISTENER_SSL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_STREAM_LISTENER_SSL))

/* --- structures --- */
struct _EvaStreamListenerSslClass 
{
  EvaStreamListenerClass stream_listener_class;
};
struct _EvaStreamListenerSsl 
{
  EvaStreamListener      stream_listener;
  char *cert_file;
  char *key_file;
  char *password;
  EvaStreamListener *underlying;
};

/* --- prototypes --- */
EvaStreamListener *eva_stream_listener_ssl_new (EvaStreamListener *underlying,
						const char        *cert_file,
						const char        *key_file);

G_END_DECLS

#endif
