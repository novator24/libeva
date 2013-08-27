#ifndef __EVA_STREAM_SSL_H_
#define __EVA_STREAM_SSL_H_

#include "../evastream.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _EvaStreamSsl EvaStreamSsl;
typedef struct _EvaStreamSslClass EvaStreamSslClass;
/* --- type macros --- */
GType eva_stream_ssl_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_STREAM_SSL			(eva_stream_ssl_get_type ())
#define EVA_STREAM_SSL(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_STREAM_SSL, EvaStreamSsl))
#define EVA_STREAM_SSL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_STREAM_SSL, EvaStreamSslClass))
#define EVA_STREAM_SSL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_STREAM_SSL, EvaStreamSslClass))
#define EVA_IS_STREAM_SSL(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_STREAM_SSL))
#define EVA_IS_STREAM_SSL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_STREAM_SSL))

/* --- structures --- */
struct _EvaStreamSslClass 
{
  EvaStreamClass stream_class;
};

typedef enum
{
  EVA_STREAM_SSL_STATE_CONSTRUCTING,
  EVA_STREAM_SSL_STATE_NORMAL,
  EVA_STREAM_SSL_STATE_SHUTTING_DOWN,
  EVA_STREAM_SSL_STATE_SHUT_DOWN,

  EVA_STREAM_SSL_STATE_ERROR
} EvaStreamSslState;

struct _EvaStreamSsl 
{
  EvaStream      stream;

  gpointer       ctx;		/* an SSL_CTX *        */
  gpointer       ssl;           /* an SSL *            */

  guint          is_client : 1;
  guint          doing_handshake : 1;
  guint          got_remote_shutdown : 1;

  /* Is this stream supposed to readable/writable? */
  guint          this_readable : 1;
  guint          this_writable : 1;

  /* What events are we waiting for on the backend? */
  guint          backend_poll_read : 1;
  guint          backend_poll_write : 1;

  /* Sometimes the opposite event is required to accomplish
     the read or write.
     
     If these are not set, then this_readable==transport_poll_read
     and this_writable==transport_poll_write. */
  guint          read_needed_to_write : 1;
  guint          write_needed_to_read : 1;
  guint          reread_length;
  guint          rewrite_length;

  guint          read_buffer_alloc;
  guint          write_buffer_alloc;
  guint          read_buffer_length;
  guint          write_buffer_length;
  guint8        *read_buffer;
  guint8        *write_buffer;


  EvaStreamSslState state;

  char    *password;
  char    *ca_file;
  char    *ca_dir;
  char    *cert_file;
  char    *key_file;

  EvaStream     *backend;     /* buffered transport layer */
  EvaStream     *transport;   /* raw transport layer */
};

/* --- prototypes --- */
EvaStream   *eva_stream_ssl_new_server   (const char   *cert_file,
					  const char   *key_file,
					  const char   *password,
					  EvaStream    *transport,
					  GError      **error);
EvaStream   *eva_stream_ssl_new_client   (const char   *cert_file,
					  const char   *key_file,
					  const char   *password,
					  EvaStream    *transport,
					  GError      **error);
EvaStream   *eva_stream_ssl_peek_backend (EvaStreamSsl *ssl);


G_END_DECLS

#endif
