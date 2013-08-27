/* NB: the transfer isn't *really* done until you're finishing uploading and/or downloading,
   but we actually ignore that;  not ignoring makes a slightly irritating circular reference danger.
 */
#ifndef __EVA_URL_TRANSFER_H_
#define __EVA_URL_TRANSFER_H_

#include "evaurl.h"
#include "../evasocketaddress.h"
#include "../evamainloop.h"
#include "../evapacket.h"

G_BEGIN_DECLS

typedef struct _GskUrlTransferClass GskUrlTransferClass;
typedef struct _GskUrlTransferRedirect GskUrlTransferRedirect;
typedef struct _GskUrlTransfer GskUrlTransfer;

GType eva_url_transfer_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_URL_TRANSFER			(eva_url_transfer_get_type ())
#define EVA_URL_TRANSFER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_URL_TRANSFER, GskUrlTransfer))
#define EVA_URL_TRANSFER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_URL_TRANSFER, GskUrlTransferClass))
#define EVA_URL_TRANSFER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_URL_TRANSFER, GskUrlTransferClass))
#define EVA_IS_URL_TRANSFER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_URL_TRANSFER))
#define EVA_IS_URL_TRANSFER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_URL_TRANSFER))

typedef enum
{
  EVA_URL_TRANSFER_ERROR_BAD_REQUEST,
  EVA_URL_TRANSFER_ERROR_BAD_NAME,
  EVA_URL_TRANSFER_ERROR_NO_SERVER,
  EVA_URL_TRANSFER_ERROR_NOT_FOUND,
  EVA_URL_TRANSFER_ERROR_SERVER_ERROR,
  EVA_URL_TRANSFER_ERROR_UNSUPPORTED,
  EVA_URL_TRANSFER_ERROR_TIMED_OUT,
  EVA_URL_TRANSFER_ERROR_REDIRECT_LOOP,
  EVA_URL_TRANSFER_REDIRECT,
  EVA_URL_TRANSFER_CANCELLED,
  EVA_URL_TRANSFER_SUCCESS
} GskUrlTransferResult;

#define EVA_URL_TRANSFER_N_RESULTS      (EVA_URL_TRANSFER_SUCCESS+1)

const char *eva_url_transfer_result_name (GskUrlTransferResult result);

typedef void (*GskUrlTransferFunc)    (GskUrlTransfer *info,
                                       gpointer        user_data);
typedef GskStream *(*GskUrlUploadFunc)(gpointer        upload_data,
                                       gssize         *size_out,
                                       GError        **error);

struct _GskUrlTransferClass
{
  GObjectClass base_class;

  /* The test() method is called before the instance
     is constructed, to give the class an opportunity to
     see if it can handle the url. */
  gboolean (*test)  (GskUrlTransferClass *transfer_class,
                     const GskUrl        *url);

  /* The start() method is called on an instance which
     has been configured.  It returns TRUE if the transfer is started.
     If it returns FALSE, an error has occurred. */
  gboolean (*start) (GskUrlTransfer      *transfer,
                     GError             **error);

  void     (*cancel)(GskUrlTransfer     *transfer);

  char    *(*get_constructing_state) (GskUrlTransfer *transfer);
  char    *(*get_running_state) (GskUrlTransfer *transfer);
  char    *(*get_done_state) (GskUrlTransfer *transfer);

  /* default impl calls eva_url_transfer_task_notify_done(ERROR_TIMEOUT) */
  void     (*timed_out)(GskUrlTransfer     *transfer);
};

struct _GskUrlTransferRedirect
{
  gboolean is_permanent;
  GskUrl *url;
  GObject *request;
  GObject *response;
  GskUrlTransferRedirect *next;
};



struct _GskUrlTransfer
{
  GObject base_instance;

  /*< public >*/
  /* --- information prepared for the handler --- */
  GskUrlTransferResult result;
  GskUrl *url;
  GSList *redirect_urls;        // XXX: unused
  GskUrlTransferRedirect *first_redirect, *last_redirect;
  GskSocketAddress *address;

  /* may be available: protocol-specific headers */
  GObject *request;     /* a GskHttpRequest probably */
  GObject *response;    /* a GskHttpResponse probably */

  GskStream *content;   /* the downloading content */

  /* the last redirect (if any) */
  GskUrl *redirect_url; /* [just a peeked version of last_redirect->url, not a ref] */
  gboolean redirect_is_permanent;

  /* ERROR status codes */
  GError *error;

  /*< protected >*/
  GskSocketAddress   *address_hint;
  guint               follow_redirects : 1;
  guint               has_timeout : 1;
  guint               timed_out : 1;

  /*< private >*/
  GskSource *timeout_source;
  guint timeout_ms;

  GskUrlTransferFunc handler;
  gpointer handler_data;
  GDestroyNotify handler_data_destroy;

  GskUrlUploadFunc    upload_func;
  gpointer            upload_data;
  GDestroyNotify      upload_destroy;

  guint transfer_state;
};

gboolean        eva_url_transfer            (GskUrl            *url,
                                             GskUrlUploadFunc   upload_func,
                                             gpointer           upload_data,
                                             GDestroyNotify     upload_destroy,
                                             GskUrlTransferFunc handler,
                                             gpointer           data,
                                             GDestroyNotify     destroy,
                                             GError           **error);

GskUrlTransfer *eva_url_transfer_new        (GskUrl             *url);

void            eva_url_transfer_set_handler(GskUrlTransfer     *transfer,
                                             GskUrlTransferFunc  handler,
                                             gpointer            data,
                                             GDestroyNotify      destroy);
void            eva_url_transfer_set_url    (GskUrlTransfer     *transfer,
                                             GskUrl             *url);
void            eva_url_transfer_set_upload (GskUrlTransfer     *transfer,
                                             GskUrlUploadFunc    func,
                                             gpointer            data,
                                             GDestroyNotify      destroy);
void      eva_url_transfer_set_upload_packet(GskUrlTransfer     *transfer,
                                             GskPacket          *packet);
void     eva_url_transfer_set_oneshot_upload(GskUrlTransfer     *transfer,
                                             GskStream          *stream,
                                             gssize              size);

void            eva_url_transfer_set_timeout(GskUrlTransfer     *transfer,
                                             guint               millis);
void          eva_url_transfer_clear_timeout(GskUrlTransfer     *transfer);

void   eva_url_transfer_set_follow_redirects(GskUrlTransfer     *transfer,
                                             gboolean            follow_redirs);
void            eva_url_transfer_set_address_hint(GskUrlTransfer     *transfer,
                                                  GskSocketAddress   *address);

/* Starting a transfer */
gboolean        eva_url_transfer_start      (GskUrlTransfer     *transfer,
                                             GError            **error);

/* Cancelling a started transfer */
void            eva_url_transfer_cancel     (GskUrlTransfer     *transfer);


char *          eva_url_transfer_get_state_string (GskUrlTransfer *transfer);


/* --- Treating a Transfer as a Stream --- */
GskStream     * eva_url_transfer_stream_new      (GskUrlTransfer *transfer,
                                                  GError        **error);

/* --- Protected API --- */
gboolean        eva_url_transfer_has_upload      (GskUrlTransfer     *transfer);
GskStream      *eva_url_transfer_create_upload   (GskUrlTransfer     *transfer,
                                                  gssize             *size_out,
                                                  GError            **error);
gboolean        eva_url_transfer_peek_expects_download_stream (GskUrlTransfer *transfer);

/* whether eva_url_transfer_notify_done() has been called */
gboolean        eva_url_transfer_is_done         (GskUrlTransfer     *transfer);

/* invoke the notification callbacks as needed */
void            eva_url_transfer_set_address     (GskUrlTransfer     *transfer,
                                                  GskSocketAddress   *addr);
gboolean        eva_url_transfer_add_redirect    (GskUrlTransfer     *transfer,
                                                  GObject            *request,
                                                  GObject            *response,
                                                  gboolean            is_permanent,
                                                  GskUrl             *dest_url);
void            eva_url_transfer_set_download    (GskUrlTransfer     *transfer,
                                                  GskStream          *content);
void            eva_url_transfer_set_request     (GskUrlTransfer     *transfer,
                                                  GObject            *request);
void            eva_url_transfer_set_response    (GskUrlTransfer     *transfer,
                                                  GObject            *response);

void            eva_url_transfer_set_error       (GskUrlTransfer     *transfer,
                                                  const GError       *error);
void            eva_url_transfer_take_error      (GskUrlTransfer     *transfer,
                                                  GError             *error);

/* NOTE: does a g_object_unref() (undoing the one in eva_url_transfer_start),
   so the transfer may be destroyed by this function. */
void            eva_url_transfer_notify_done     (GskUrlTransfer     *transfer,
                                                  GskUrlTransferResult result);

/* Registering a transfer type */
void            eva_url_transfer_class_register  (GskUrlScheme            scheme,
                                                  GskUrlTransferClass    *transfer_class);

G_END_DECLS

#endif
