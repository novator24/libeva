#include "evaopensslbiostream.h"
#include "../evabufferstream.h"

#define MONITOR_BUFFER_STREAM_SHUTDOWN  0

typedef struct _EvaBufferStreamOpenssl EvaBufferStreamOpenssl;
typedef struct _EvaBufferStreamOpensslClass EvaBufferStreamOpensslClass;
GType eva_buffer_stream_openssl_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_BUFFER_STREAM_OPENSSL			(eva_buffer_stream_openssl_get_type ())
#define EVA_BUFFER_STREAM_OPENSSL(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_BUFFER_STREAM_OPENSSL, EvaBufferStreamOpenssl))
#define EVA_BUFFER_STREAM_OPENSSL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_BUFFER_STREAM_OPENSSL, EvaBufferStreamOpensslClass))
#define EVA_BUFFER_STREAM_OPENSSL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_BUFFER_STREAM_OPENSSL, EvaBufferStreamOpensslClass))
#define EVA_IS_BUFFER_STREAM_OPENSSL(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_BUFFER_STREAM_OPENSSL))
#define EVA_IS_BUFFER_STREAM_OPENSSL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_BUFFER_STREAM_OPENSSL))

struct _EvaBufferStreamOpensslClass 
{
  EvaBufferStreamClass buffer_stream_class;
};
struct _EvaBufferStreamOpenssl 
{
  EvaBufferStream      buffer_stream;
  BIO *bio;
};
static GObjectClass *parent_class = NULL;

/* --- functions --- */
#if MONITOR_BUFFER_STREAM_SHUTDOWN
static gboolean
eva_buffer_stream_openssl_shutdown_read   (EvaIO      *io,
				           GError    **error)
{
  g_message("eva_buffer_stream_openssl_shutdown_read: %p",io);
  return EVA_IO_CLASS (parent_class)->shutdown_read (io,error);
}

static gboolean
eva_buffer_stream_openssl_shutdown_write  (EvaIO      *io,
				           GError    **error)
{
  g_message("eva_buffer_stream_openssl_shutdown_write: %p",io);
  return EVA_IO_CLASS (parent_class)->shutdown_write (io,error);
}
#endif

static void
eva_buffer_stream_openssl_init (EvaBufferStreamOpenssl *buffer_stream_openssl)
{
}
static void
eva_buffer_stream_openssl_class_init (EvaBufferStreamOpensslClass *class)
{
  parent_class = g_type_class_peek_parent (class);
#if MONITOR_BUFFER_STREAM_SHUTDOWN
  EVA_IO_CLASS (class)->shutdown_read = eva_buffer_stream_openssl_shutdown_read;
  EVA_IO_CLASS (class)->shutdown_write = eva_buffer_stream_openssl_shutdown_write;
#endif
}

GType eva_buffer_stream_openssl_get_type()
{
  static GType buffer_stream_openssl_type = 0;
  if (!buffer_stream_openssl_type)
    {
      static const GTypeInfo buffer_stream_openssl_info =
      {
	sizeof(EvaBufferStreamOpensslClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_buffer_stream_openssl_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EvaBufferStreamOpenssl),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_buffer_stream_openssl_init,
	NULL		/* value_table */
      };
      buffer_stream_openssl_type = g_type_register_static (EVA_TYPE_BUFFER_STREAM,
                                                  "EvaBufferStreamOpenssl",
						  &buffer_stream_openssl_info, 0);
    }
  return buffer_stream_openssl_type;
}

/* --- the BIO --- */
#if 0		/* DEBUG */
  #ifdef G_HAVE_ISO_VARARGS
  #define DEBUG_BIO(...)  g_message("DEBUG BIO: " __VA_ARGS__)
  #elif defined(G_HAVE_GNUC_VARARGS)
  #define DEBUG_BIO(format...) g_message("DEBUG_BIO: " format)
  #else
  #define DEBUG_BIO g_message
  #endif
#else
  #ifdef G_HAVE_ISO_VARARGS
  #define DEBUG_BIO(...)  
  #elif defined(G_HAVE_GNUC_VARARGS)
  #define DEBUG_BIO(format...)
  #else
  #define DEBUG_BIO (void)
  #endif
#endif

static int 
bio_eva_stream_pair_bwrite (BIO *bio,
		       const char *out,
		       int length)
{
  EvaBufferStream *buffer_stream = EVA_BUFFER_STREAM (bio->ptr);
  DEBUG_BIO("bio_eva_stream_pair_bwrite: writing %d bytes to read-buffer of backend", length);
  eva_buffer_append (eva_buffer_stream_peek_read_buffer (buffer_stream), out, length);
  eva_buffer_stream_read_buffer_changed (buffer_stream);
  return length;
}

static int 
bio_eva_stream_pair_bread (BIO *bio,
		      char *in,
		      int max_length)
{
  EvaBufferStream *buffer_stream = EVA_BUFFER_STREAM (bio->ptr);
  guint length = eva_buffer_read (eva_buffer_stream_peek_write_buffer (buffer_stream), in, max_length);
  DEBUG_BIO("bio_eva_stream_pair_bread: read %u bytes of %d bytes from backend write buffer", length, max_length);
  if (length > 0)
    eva_buffer_stream_write_buffer_changed (buffer_stream);
  return length;
}

static long 
bio_eva_stream_pair_ctrl (BIO  *bio,
		     int   cmd,
		     long  num,
		     void *ptr)
{
  EvaBufferStreamOpenssl *openssl_buffer_stream = EVA_BUFFER_STREAM_OPENSSL (bio->ptr);
  g_assert (openssl_buffer_stream->bio == bio);

  DEBUG_BIO("bio_eva_stream_pair_ctrl: called with cmd=%d", cmd);

  switch (cmd)
    {
    case BIO_CTRL_DUP:
    case BIO_CTRL_FLUSH:
    case BIO_CTRL_PENDING:
    case BIO_CTRL_WPENDING:
      return 1;
    }

  /* -1 seems more appropriate, but this is
     what bss_fd returns when it doesn't know the cmd. */
  return 0;
}

static int 
bio_eva_stream_pair_create (BIO *bio)
{
  DEBUG_BIO("bio_eva_stream_pair_create (%p)", bio);
  return TRUE;
}

static int 
bio_eva_stream_pair_destroy (BIO *bio)
{
  EvaBufferStream *buffer_stream = EVA_BUFFER_STREAM (bio->ptr);
  DEBUG_BIO("bio_eva_stream_pair_destroy (%p)", bio);
  if (buffer_stream == NULL)
    return FALSE;
  g_object_unref (buffer_stream);
  bio->ptr = NULL;
  return TRUE;
}

static BIO_METHOD bio_method_eva_stream_pair =
{
  22,				/* type:  this is quite a hack */
  "EvaStream-BIO",		/* name */
  bio_eva_stream_pair_bwrite,	/* bwrite */
  bio_eva_stream_pair_bread,	/* bread */
  NULL,				/* bputs */
  NULL,				/* bgets */
  bio_eva_stream_pair_ctrl,	/* ctrl */
  bio_eva_stream_pair_create,	/* create */
  bio_eva_stream_pair_destroy,	/* destroy */
  NULL				/* callback_ctrl */
};



/**
 * eva_openssl_bio_stream_pair:
 * @bio_out: return location for a newly allocated BIO* object,
 * (a BIO* is the I/O abstraction used in openssl).
 * @stream_out: return location fora newly allocated EvaBufferStream.
 *
 * The stream and BIO and hooked up such 
 * that writes to the stream will be read
 * from the BIO, and vice versa.
 *
 * The caller is responsible for handling the buffered_read
 * and buffered_write hooks.
 *
 * returns: whether the pair could be made.
 */
gboolean
eva_openssl_bio_stream_pair (BIO              **bio_out,
			     EvaBufferStream  **stream_out)
{
  EvaBufferStreamOpenssl *openssl_stream = g_object_new (EVA_TYPE_BUFFER_STREAM_OPENSSL, NULL);
  EvaStream *stream = EVA_STREAM (openssl_stream);
  *bio_out = BIO_new (&bio_method_eva_stream_pair);
  (*bio_out)->ptr = g_object_ref (stream);
  (*bio_out)->init = TRUE;		/// HMM...
  *stream_out = EVA_BUFFER_STREAM (stream);
  openssl_stream->bio = *bio_out;
  return TRUE;
}
