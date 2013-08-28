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

typedef struct _EvaUrlTransferClass EvaUrlTransferClass;
typedef struct _EvaUrlTransferRedirect EvaUrlTransferRedirect;
typedef struct _EvaUrlTransfer EvaUrlTransfer;

GType eva_url_transfer_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_URL_TRANSFER			(eva_url_transfer_get_type ())
#define EVA_URL_TRANSFER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_URL_TRANSFER, EvaUrlTransfer))
#define EVA_URL_TRANSFER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_URL_TRANSFER, EvaUrlTransferClass))
#define EVA_URL_TRANSFER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_URL_TRANSFER, EvaUrlTransferClass))
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
} EvaUrlTransferResult;

#define EVA_URL_TRANSFER_N_RESULTS      (EVA_URL_TRANSFER_SUCCESS+1)

const char *eva_url_transfer_result_name (EvaUrlTransferResult result);

typedef void (*EvaUrlTransferFunc)    (EvaUrlTransfer *info,
                                       gpointer        user_data);
typedef EvaStream *(*EvaUrlUploadFunc)(gpointer        upload_data,
                                       gssize         *size_out,
                                       GError        **error);

struct _EvaUrlTransferClass
{
  GObjectClass base_class;

  /* The test() method is called before the instance
     is constructed, to give the class an opportunity to
     see if it can handle the url. */
  gboolean (*test)  (EvaUrlTransferClass *transfer_class,
                     const EvaUrl        *url);

  /* The start() method is called on an instance which
     has been configured.  It returns TRUE if the transfer is started.
     If it returns FALSE, an error has occurred. */
  gboolean (*start) (EvaUrlTransfer      *transfer,
                     GError             **error);

  void     (*cancel)(EvaUrlTransfer     *transfer);

  char    *(*get_constructing_state) (EvaUrlTransfer *transfer);
  char    *(*get_running_state) (EvaUrlTransfer *transfer);
  char    *(*get_done_state) (EvaUrlTransfer *transfer);

  /* default impl calls eva_url_transfer_task_notify_done(ERROR_TIMEOUT) */
  void     (*timed_out)(EvaUrlTransfer     *transfer);
};

struct _EvaUrlTransferRedirect
{
  gboolean is_permanent;
  EvaUrl *url;
  GObject *request;
  GObject *response;
  EvaUrlTransferRedirect *next;
};



struct _EvaUrlTransfer
{
  GObject base_instance;

  /*< public >*/
  /* --- information prepared for the handler --- */
  EvaUrlTransferResult result;
  EvaUrl *url;
  GSList *redirect_urls;        // XXX: unused
  EvaUrlTransferRedirect *first_redirect, *last_redirect;
  EvaSocketAddress *address;

  /* may be available: protocol-specific headers */
  GObject *request;     /* a EvaHttpRequest probably */
  GObject *response;    /* a EvaHttpResponse probably */

  EvaStream *content;   /* the downloading content */

  /* the last redirect (if any) */
  EvaUrl *redirect_url; /* [just a peeked version of last_redirect->url, not a ref] */
  gboolean redirect_is_permanent;

  /* ERROR status codes */
  GError *error;

  /*< protected >*/
  EvaSocketAddress   *address_hint;
  guint               follow_redirects : 1;
  guint               has_timeout : 1;
  guint               timed_out : 1;

  /*< private >*/
  EvaSource *timeout_source;
  guint timeout_ms;

  EvaUrlTransferFunc handler;
  gpointer handler_data;
  GDestroyNotify handler_data_destroy;

  EvaUrlUploadFunc    upload_func;
  gpointer            upload_data;
  GDestroyNotify      upload_destroy;

  guint transfer_state;
};

gboolean        eva_url_transfer            (EvaUrl            *url,
                                             EvaUrlUploadFunc   upload_func,
                                             gpointer           upload_data,
                                             GDestroyNotify     upload_destroy,
                                             EvaUrlTransferFunc handler,
                                             gpointer           data,
                                             GDestroyNotify     destroy,
                                             GError           **error);

EvaUrlTransfer *eva_url_transfer_new        (EvaUrl             *url);

void            eva_url_transfer_set_handler(EvaUrlTransfer     *transfer,
                                             EvaUrlTransferFunc  handler,
                                             gpointer            data,
                                             GDestroyNotify      destroy);
void            eva_url_transfer_set_url    (EvaUrlTransfer     *transfer,
                                             EvaUrl             *url);
void            eva_url_transfer_set_upload (EvaUrlTransfer     *transfer,
                                             EvaUrlUploadFunc    func,
                                             gpointer            data,
                                             GDestroyNotify      destroy);
void      eva_url_transfer_set_upload_packet(EvaUrlTransfer     *transfer,
                                             EvaPacket          *packet);
void     eva_url_transfer_set_oneshot_upload(EvaUrlTransfer     *transfer,
                                             EvaStream          *stream,
                                             gssize              size);

void            eva_url_transfer_set_timeout(EvaUrlTransfer     *transfer,
                                             guint               millis);
void          eva_url_transfer_clear_timeout(EvaUrlTransfer     *transfer);

void   eva_url_transfer_set_follow_redirects(EvaUrlTransfer     *transfer,
                                             gboolean            follow_redirs);
void            eva_url_transfer_set_address_hint(EvaUrlTransfer     *transfer,
                                                  EvaSocketAddress   *address);

/* Starting a transfer */
gboolean        eva_url_transfer_start      (EvaUrlTransfer     *transfer,
                                             GError            **error);

/* Cancelling a started transfer */
void            eva_url_transfer_cancel     (EvaUrlTransfer     *transfer);


char *          eva_url_transfer_get_state_string (EvaUrlTransfer *transfer);


/* --- Treating a Transfer as a Stream --- */
EvaStream     * eva_url_transfer_stream_new      (EvaUrlTransfer *transfer,
                                                  GError        **error);

/* --- Protected API --- */
gboolean        eva_url_transfer_has_upload      (EvaUrlTransfer     *transfer);
EvaStream      *eva_url_transfer_create_upload   (EvaUrlTransfer     *transfer,
                                                  gssize             *size_out,
                                                  GError            **error);
gboolean        eva_url_transfer_peek_expects_download_stream (EvaUrlTransfer *transfer);

/* whether eva_url_transfer_notify_done() has been called */
gboolean        eva_url_transfer_is_done         (EvaUrlTransfer     *transfer);

/* invoke the notification callbacks as needed */
void            eva_url_transfer_set_address     (EvaUrlTransfer     *transfer,
                                                  EvaSocketAddress   *addr);
gboolean        eva_url_transfer_add_redirect    (EvaUrlTransfer     *transfer,
                                                  GObject            *request,
                                                  GObject            *response,
                                                  gboolean            is_permanent,
                                                  EvaUrl             *dest_url);
void            eva_url_transfer_set_download    (EvaUrlTransfer     *transfer,
                                                  EvaStream          *content);
void            eva_url_transfer_set_request     (EvaUrlTransfer     *transfer,
                                                  GObject            *request);
void            eva_url_transfer_set_response    (EvaUrlTransfer     *transfer,
                                                  GObject            *response);

void            eva_url_transfer_set_error       (EvaUrlTransfer     *transfer,
                                                  const GError       *error);
void            eva_url_transfer_take_error      (EvaUrlTransfer     *transfer,
                                                  GError             *error);

/* NOTE: does a g_object_unref() (undoing the one in eva_url_transfer_start),
   so the transfer may be destroyed by this function. */
void            eva_url_transfer_notify_done     (EvaUrlTransfer     *transfer,
                                                  EvaUrlTransferResult result);

/* Registering a transfer type */
void            eva_url_transfer_class_register  (EvaUrlScheme            scheme,
                                                  EvaUrlTransferClass    *transfer_class);

G_END_DECLS

#endif
