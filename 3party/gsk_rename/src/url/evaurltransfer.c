#include <string.h>
#include "evaurltransfer.h"
#include "../evamemory.h"

/**
 * eva_url_transfer_result_name:
 *
 * @result: the enumeration value.
 *
 * Convert a EvaUrlTransferResult value
 * into a human-readable string.
 *
 * returns: the constant string.
 */
const char *
eva_url_transfer_result_name (EvaUrlTransferResult result)
{
  switch (result)
    {
    case EVA_URL_TRANSFER_ERROR_BAD_REQUEST:
      return "Error: Bad Request";
    case EVA_URL_TRANSFER_ERROR_BAD_NAME:
      return "Error: Bad Name";
    case EVA_URL_TRANSFER_ERROR_NO_SERVER:
      return "Error: No Server";
    case EVA_URL_TRANSFER_ERROR_NOT_FOUND:
      return "Error: Not Found";
    case EVA_URL_TRANSFER_ERROR_SERVER_ERROR:
      return "Error: Server Error";
    case EVA_URL_TRANSFER_ERROR_UNSUPPORTED:
      return "Error: Unsupported";
    case EVA_URL_TRANSFER_ERROR_TIMED_OUT:
      return "Error: Timed Out";
    case EVA_URL_TRANSFER_ERROR_REDIRECT_LOOP:
      return "Error: Redirect Loop";
    case EVA_URL_TRANSFER_REDIRECT:
      return "Redirect";
    case EVA_URL_TRANSFER_CANCELLED:
      return "Cancelled";
    case EVA_URL_TRANSFER_SUCCESS:
      return "Success";
    default:
      g_warning ("requested name of invalid transfer result %u", result);
      g_return_val_if_reached (NULL);
    }
}

G_DEFINE_TYPE(EvaUrlTransfer, eva_url_transfer, G_TYPE_OBJECT);

typedef enum
{
  EVA_URL_TRANSFER_STATE_CONSTRUCTING,
  EVA_URL_TRANSFER_STATE_STARTED,
  EVA_URL_TRANSFER_STATE_DONE,
  EVA_URL_TRANSFER_STATE_ERROR
} EvaUrlTransferState;

static inline void
eva_url_transfer_redirect_free_1 (EvaUrlTransferRedirect *redirect)
{
  g_object_unref (redirect->url);
  if (redirect->request)
    g_object_unref (redirect->request);
  if (redirect->response)
    g_object_unref (redirect->response);
  g_free (redirect);
}

static void
eva_url_transfer_finalize (GObject *object)
{
  EvaUrlTransfer *transfer = EVA_URL_TRANSFER (object);
  EvaUrlTransferRedirect *redir_at;
  g_assert (transfer->transfer_state != EVA_URL_TRANSFER_STATE_STARTED);

  if (transfer->url)
    g_object_unref (transfer->url);
  redir_at = transfer->first_redirect;
  while (redir_at)
    {
      EvaUrlTransferRedirect *next = redir_at->next;
      eva_url_transfer_redirect_free_1 (redir_at);
      redir_at = next;
    }
  if (transfer->address)
    g_object_unref (transfer->address);
  if (transfer->address_hint)
    g_object_unref (transfer->address_hint);

  /* may be available: protocol-specific headers */
  if (transfer->request)
    g_object_unref (transfer->request);
  if (transfer->response)
    g_object_unref (transfer->response);

  if (transfer->content)
    g_object_unref (transfer->content);
  if (transfer->upload_destroy != NULL)
    (*transfer->upload_destroy) (transfer->upload_data);

  g_clear_error (&transfer->error);

  G_OBJECT_CLASS (eva_url_transfer_parent_class)->finalize (object);
}

static void
eva_url_transfer_real_timed_out (EvaUrlTransfer *transfer)
{
  eva_url_transfer_take_error (transfer,
                               g_error_new (EVA_G_ERROR_DOMAIN,
                                            EVA_ERROR_OPERATION_TIMED_OUT,
                                            "Transfer with URL timed out"));
  if (transfer->transfer_state != EVA_URL_TRANSFER_STATE_DONE)
    eva_url_transfer_notify_done (transfer, EVA_URL_TRANSFER_ERROR_TIMED_OUT);
}

static char    *
eva_url_transfer_real_get_constructing_state (EvaUrlTransfer *transfer)
{
  if (transfer->url)
    {
      char *url_str = eva_url_to_string (transfer->url);
      char *rv = g_strdup_printf ("NOT STARTED: %s", url_str);
      g_free (url_str);
      return rv;
    }
  return g_strdup ("NOT STARTED: (no url)");
}

static char    *
eva_url_transfer_real_get_running_state (EvaUrlTransfer *transfer)
{
  if (transfer->url)
    {
      char *url_str = eva_url_to_string (transfer->url);
      char *rv = g_strdup_printf ("RUNNING: %s", url_str);
      g_free (url_str);
      return rv;
    }
  return g_strdup ("RUNNING: (no url!?!)");
}

static char    *
eva_url_transfer_real_get_done_state (EvaUrlTransfer *transfer)
{
  if (transfer->url)
    {
      char *url_str = eva_url_to_string (transfer->url);
      char *rv = g_strdup_printf ("DONE: %s: %s", url_str,
                          eva_url_transfer_result_name (transfer->result));
      g_free (url_str);
      return rv;
    }
  return g_strdup_printf ("DONE: [no url]: %s",
                          eva_url_transfer_result_name (transfer->result));
}

static void
eva_url_transfer_init (EvaUrlTransfer *transfer)
{
  transfer->transfer_state = EVA_URL_TRANSFER_STATE_CONSTRUCTING;
  transfer->follow_redirects = 1;
}

static void
eva_url_transfer_class_init (EvaUrlTransferClass *transfer_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (transfer_class);
  object_class->finalize = eva_url_transfer_finalize;
  transfer_class->timed_out = eva_url_transfer_real_timed_out;
  transfer_class->get_constructing_state = eva_url_transfer_real_get_constructing_state;
  transfer_class->get_running_state = eva_url_transfer_real_get_running_state;
  transfer_class->get_done_state = eva_url_transfer_real_get_done_state;
}


/**
 * eva_url_transfer:
 * @url: the URL to which to upload or from which to download data.
 * @upload_func: optional function that can create the upload's content
 * as a #EvaStream.
 * @upload_data: data which can be used by the upload function.
 * @upload_destroy: optional function that will be notified when upload()
 * will no longer be called (note that the streams it created may
 * still be extant though).
 * @handler: function to be called with the transfer request is
 * done.  (The transfer content itself is just provided as a stream
 * though-- only after reading the stream is the transfer truly done)
 * This function may also be called in a number of error cases.
 * @data: data to pass to the handler function.
 * @destroy: function to call when you are done with data.
 * @error: place to put the error if anything goes wrong.
 *
 * Begin a upload and/or download with a URL.
 * There is no way to cancel this transfer.
 *
 * If you wish to perform an upload,
 * provide a function that can create the stream of content to
 * upload on demand.  Note that the upload_destroy() method
 * is called only once the transfer is done and all the upload streams
 * are finalized.  Therefore, you can assume that the upload_data
 * will be available for all your upload-streams.
 * 
 * The handler/data/destroy triple is used for result notification.
 * handler() is always invoked exactly once.  To find out how things
 * went, the handler() should almost always start by
 * examining transfer->result.
 *
 * returns: whether the transfer began.
 * Unsupported URL schemes and malformed URLs are the
 * most common ways for this function to fail.
 */
gboolean
eva_url_transfer            (EvaUrl            *url,
                             EvaUrlUploadFunc   upload_func,
                             gpointer           upload_data,
                             GDestroyNotify     upload_destroy,
                             EvaUrlTransferFunc handler,
                             gpointer           data,
                             GDestroyNotify     destroy,
                             GError           **error)
{
  EvaUrlTransfer *transfer = eva_url_transfer_new (url);
  if (transfer == NULL)
    {
      g_set_error (error,
                   EVA_G_ERROR_DOMAIN,
                   EVA_ERROR_INVALID_ARGUMENT,
                   "could not create Transfer object for url of scheme %s", url->scheme_name);
      return FALSE;
    }
  eva_url_transfer_set_handler (transfer, handler, data, destroy);
  if (upload_func != NULL)
    eva_url_transfer_set_upload (transfer, upload_func, upload_data, upload_destroy);
  if (!eva_url_transfer_start (transfer, error))
    return FALSE;
  g_object_unref (transfer);
  return TRUE;
}

static gboolean
handle_timeout (gpointer data)
{
  EvaUrlTransfer *transfer = EVA_URL_TRANSFER (data);
  EvaUrlTransferClass *class = EVA_URL_TRANSFER_GET_CLASS (transfer);
  g_return_val_if_fail (transfer->transfer_state == EVA_URL_TRANSFER_STATE_STARTED, FALSE);
  transfer->timeout_source = NULL;
  transfer->timed_out = TRUE;

  /* hold a reference to transfer temporarily to ensure
     that the transfer doesn't die in the middle */
  g_object_ref (transfer);
  class->timed_out (transfer);
  g_object_unref (transfer); /* transfer may now be dead */
  return FALSE;
}

/**
 * eva_url_transfer_start:
 * @transfer: the Transfer to affect.
 * @error: place to put the error if anything goes wrong.
 *
 * Begin the upload and/or download.  (Maybe start with name-lookup).
 *
 * returns: whether the transfer started successfully.
 * If it returns TRUE, you are guaranteed to receive your
 * done-notification.  If is returns FALSE, you will definitely not
 * receive done-notification.
 */
gboolean
eva_url_transfer_start      (EvaUrlTransfer     *transfer,
                             GError            **error)
{
  EvaUrlTransferClass *class = EVA_URL_TRANSFER_GET_CLASS (transfer);
  g_assert (class->start != NULL);
  g_return_val_if_fail (transfer->transfer_state == EVA_URL_TRANSFER_STATE_CONSTRUCTING, FALSE);
  g_object_ref (transfer);
  transfer->transfer_state = EVA_URL_TRANSFER_STATE_STARTED;
  if (!class->start (transfer, error))
    {
      transfer->transfer_state = EVA_URL_TRANSFER_STATE_ERROR;
      g_object_unref (transfer);
      return FALSE;
    }
  if (transfer->has_timeout
   && transfer->transfer_state == EVA_URL_TRANSFER_STATE_STARTED)
    {
      transfer->timeout_source = eva_main_loop_add_timer (eva_main_loop_default (),
                                                          handle_timeout,
                                                          transfer,
                                                          NULL,
                                                          transfer->timeout_ms,
                                                          -1);
    }
  return TRUE;
}

/**
 * eva_url_transfer_set_handler:
 * @transfer: the Transfer to affect.
 * @handler: function to be called with the transfer request is
 * done.  (The transfer content itself is just provided as a stream
 * though-- only after reading the stream is the transfer truly done)
 * This function may also be called in a number of error cases.
 * @data: data to pass to the handler function.
 * @destroy: function to call when you are done with data.
 *
 * The handler/data/destroy triple is used for result notification.
 * handler() is always invoked exactly once.  To find out how things
 * went, the handler() should almost always start by
 * examining transfer->result.
 */
void
eva_url_transfer_set_handler(EvaUrlTransfer     *transfer,
                             EvaUrlTransferFunc  handler,
                             gpointer            data,
                             GDestroyNotify      destroy)
{
  g_return_if_fail (transfer->transfer_state == EVA_URL_TRANSFER_STATE_CONSTRUCTING);
  g_return_if_fail (transfer->handler == NULL);
  transfer->handler = handler;
  transfer->handler_data = data;
  transfer->handler_data_destroy = destroy;
}

/**
 * eva_url_transfer_set_url:
 * @transfer: the Transfer to affect.
 * @url: the URL to which to upload or from which to download data.
 *
 * Set the URL that is the target of this transfer.
 * This can only be done once, before the transfer
 * is started.
 *
 * You seldom need to use this function, as it
 * is called by eva_url_transfer_new().
 */
void
eva_url_transfer_set_url    (EvaUrlTransfer     *transfer,
                             EvaUrl             *url)
{
  g_return_if_fail (transfer->transfer_state == EVA_URL_TRANSFER_STATE_CONSTRUCTING);
  g_return_if_fail (transfer->url == NULL);
  g_return_if_fail (EVA_IS_URL (url));
  transfer->url = g_object_ref (url);
}

/**
 * eva_url_transfer_set_timeout:
 * @transfer: the Transfer to affect.
 * @millis: milliseconds to wait before aborting the transfer.
 *
 * Set the timeout on the download.
 *
 * This can be used to avoid hanging on slow servers.
 *
 * This must be called before the transfer is started
 * (with eva_url_transfer_start).
 */
void
eva_url_transfer_set_timeout(EvaUrlTransfer     *transfer,
                             guint               millis)
{
  g_return_if_fail (transfer->transfer_state == EVA_URL_TRANSFER_STATE_CONSTRUCTING);
  transfer->has_timeout = 1;
  transfer->timeout_ms = millis;
}

/**
 * eva_url_transfer_clear_timeout:
 * @transfer: the Transfer to affect.
 *
 * Clear the timeout on the download.
 *
 * This must be called before the transfer is started
 * (with eva_url_transfer_start).
 */
void
eva_url_transfer_clear_timeout(EvaUrlTransfer     *transfer)
{
  g_return_if_fail (transfer->transfer_state == EVA_URL_TRANSFER_STATE_CONSTRUCTING);
  transfer->has_timeout = 0;
}

/**
 * eva_url_transfer_set_follow_redirects:
 * @transfer: the Transfer to affect.
 * @follow_redirs: whether to follow redirect responses.
 *
 * Configure how the transfer will behave when it encounters
 * redirection responses.
 *
 * The default behavior is to follow redirects,
 * adding them to the list of redirects, but not notifying the
 * user until we reach a real page (or error).
 *
 * If follow_redirects is FALSE, then we are done
 * even if the download led to a redirect.
 */
void
eva_url_transfer_set_follow_redirects(EvaUrlTransfer     *transfer,
                                      gboolean            follow_redirs)
{
  g_return_if_fail (transfer->transfer_state == EVA_URL_TRANSFER_STATE_CONSTRUCTING);
  transfer->follow_redirects = follow_redirs ? 1 : 0;
}


/**
 * eva_url_transfer_set_address_hint:
 * @transfer: the Transfer to affect.
 * @address: the socket-address to use for connecting,
 * possibly with the wrong port (the port will be overridden by
 * the URL's port).
 *
 * To avoid DNS lookups in very bulky transfer situations,
 * DNS may be bypassed and replaced with this address.
 *
 * Chances are, you want to suppress redirects to:
 * otherwise, DNS may be used on the redirected URLs.
 */
void
eva_url_transfer_set_address_hint (EvaUrlTransfer *transfer,
                                   EvaSocketAddress *address)
{
  g_return_if_fail (transfer->transfer_state == EVA_URL_TRANSFER_STATE_CONSTRUCTING);
  g_return_if_fail (transfer->address_hint == NULL);
  transfer->address_hint = g_object_ref (address);
}


/**
 * eva_url_transfer_set_upload:
 * @transfer: the Transfer to affect.
 * @func: function that can create the upload's content
 * as a #EvaStream.
 * @data: data which can be used by the upload function.
 * @destroy: optional function that will be notified when upload()
 * will no longer be called (note that the streams it created may
 * still be extant though).
 *
 * Set the upload stream as generally as possible.
 * Actually you must provide a function
 * that can make an upload stream on demand--
 * this is necessary to get redirects right.
 *
 * The destroy() function will be called after no more upload-streams
 * need to be created-- it is quite possible that not all upload-streams
 * have been finalized by the time the destroy() is invoked.
 *
 * If you don't care about redirects, you can
 * use eva_url_transfer_set_oneshot_upload().
 *
 * If you have a slab of memory that you want to use as the upload stream,
 * consider using eva_url_transfer_set_upload_packet().
 */
void
eva_url_transfer_set_upload (EvaUrlTransfer     *transfer,
                             EvaUrlUploadFunc    func,
                             gpointer            data,
                             GDestroyNotify      destroy)
{
  g_return_if_fail (transfer->transfer_state == EVA_URL_TRANSFER_STATE_CONSTRUCTING);
  g_return_if_fail (transfer->upload_func == NULL);
  g_return_if_fail (func != NULL);
  transfer->upload_func = func;
  transfer->upload_data = data;
  transfer->upload_destroy = destroy;
}

/**
 * eva_url_transfer_set_upload_packet:
 * @transfer: the Transfer to affect.
 * @packet: the EvaPacket containing the upload content as data.
 *
 * Set the upload stream in an easy, reliable way using a EvaPacket.
 */
static EvaStream *
make_packet_into_stream (gpointer data,
                         gssize  *size_out,
                         GError **error)
{
  EvaPacket *packet = data;
  EvaStream *rv = eva_memory_slab_source_new (packet->data, packet->len,
                                              (GDestroyNotify) eva_packet_unref,
                                              eva_packet_ref (packet));
  *size_out = packet->len;
  return rv;
}

void
eva_url_transfer_set_upload_packet (EvaUrlTransfer *transfer,
                                    EvaPacket      *packet)
{
  eva_url_transfer_set_upload (transfer,
                               make_packet_into_stream,
                               eva_packet_ref (packet),
                               (GDestroyNotify) eva_packet_unref);
}

/**
 * eva_url_transfer_set_oneshot_upload:
 * @transfer: the Transfer to affect.
 * @size: the length of the stream in bytes, or -1 if you don't know.
 * @stream: the upload content stream.
 *
 * Set the content to upload to the remote URL,
 * as a #EvaStream.
 *
 * Since streams can only be read once,
 * this method only works on URLs that do not require
 * redirection.
 */
typedef struct
{
  EvaStream *stream;
  gssize size;
} ReturnStreamOnce;

static EvaStream *
return_stream_once      (gpointer data,
                         gssize  *size_out,
                         GError **error)
{
  ReturnStreamOnce *once = data;
  EvaStream *rv;
  if (once->stream == NULL)
    {
      g_set_error (error,
                   EVA_G_ERROR_DOMAIN,
                   EVA_ERROR_TOO_MANY_LINKS,
                   "one-shot upload transfer was redirected: cannot re-upload data");
      return NULL;
    }
  rv = once->stream;
  once->stream = NULL;
  *size_out = once->size;
  return rv;
}
static void
destroy_return_stream_once (gpointer data)
{
  ReturnStreamOnce *once = data;
  if (once->stream)
    g_object_unref (once->stream);
  g_free (once);
}

void
eva_url_transfer_set_oneshot_upload (EvaUrlTransfer *transfer,
                                     EvaStream      *stream,
                                     gssize          size)
{
  ReturnStreamOnce *once;
  g_return_if_fail (EVA_IS_STREAM (stream));
  once = g_new (ReturnStreamOnce, 1);
  once->stream = g_object_ref (stream);
  once->size = size;
  eva_url_transfer_set_upload (transfer,
                               return_stream_once,
                               once,
                               destroy_return_stream_once);
}

/**
 * eva_url_transfer_cancel:
 * @transfer: the Transfer to affect.
 *
 * Abort a running transfer.
 *
 * If you registered a handler, it will be called with 
 * result EVA_URL_TRANSFER_CANCELLED.
 */
void
eva_url_transfer_cancel     (EvaUrlTransfer     *transfer)
{
  EvaUrlTransferClass *class = EVA_URL_TRANSFER_GET_CLASS (transfer);
  g_return_if_fail (transfer->transfer_state == EVA_URL_TRANSFER_STATE_STARTED);
  if (class->cancel == NULL)
    {
      g_warning ("%s does not implement cancel()!", G_OBJECT_CLASS_NAME (class));
      return;
    }
  class->cancel (transfer);
}

/* --- Protected API --- */
/**
 * eva_url_transfer_has_upload:
 * @transfer: the Transfer to query.
 *
 * Figure out whether this transfer has upload data.
 *
 * This function should only be needed by implementors
 * of types of EvaUrlTransfer.
 *
 * returns: whether the transfer has upload data.
 */
gboolean
eva_url_transfer_has_upload      (EvaUrlTransfer     *transfer)
{
  return transfer->upload_func != NULL;
}

/**
 * eva_url_transfer_create_upload:
 * @transfer: the Transfer to use.
 * @size_out: the size of the stream in bytes, or -1 if the size is unknown.
 * @error: optional location to store the #GError if there is a problem.
 *
 * Create a upload stream for this transfer based on the user's creator
 * function.
 *
 * This function should only be needed by implementors
 * of types of EvaUrlTransfer.
 *
 * returns: a newly allocated #EvaStream, or NULL if an error occurs.
 */
EvaStream *
eva_url_transfer_create_upload   (EvaUrlTransfer     *transfer,
                                  gssize             *size_out,
                                  GError            **error)
{
  g_return_val_if_fail (transfer->upload_func != NULL, NULL);
  *size_out = -1;
  return transfer->upload_func (transfer->upload_data, size_out, error);
}

/**
 * eva_url_transfer_peek_expects_download_stream:
 * @transfer: the Transfer to use.
 * returns: whether this transfer has a download handler.
 *
 * This function can be used to see if download-content is expected.
 *
 * This function should only be needed by implementors
 * of types of EvaUrlTransfer.
 */
gboolean
eva_url_transfer_peek_expects_download_stream (EvaUrlTransfer *transfer)
{
  g_return_val_if_fail (transfer->transfer_state == EVA_URL_TRANSFER_STATE_STARTED, FALSE);
  return transfer->handler != NULL;
}

/**
 * eva_url_transfer_set_address:
 * @transfer: the Transfer to affect.
 * @addr: the address of the host whose lookup was completed.
 *
 * Set the socket-address for informational purposes.
 * This is occasionally interesting to the user of the Transfer.
 *
 * This function should only be needed by implementors
 * of types of EvaUrlTransfer.
 */
void
eva_url_transfer_set_address     (EvaUrlTransfer     *transfer,
                                  EvaSocketAddress   *addr)
{
  g_object_ref (addr);
  if (transfer->address)
    g_object_unref (transfer->address);
  transfer->address = addr;
}

static inline gboolean
strings_equal (const char *a, const char *b)
{
  if (a == NULL)
    return b == NULL;
  else if (b == NULL)
    return FALSE;
  else
    return strcmp (a, b) == 0;
}

static gboolean
urls_equal_up_to_fragment (const EvaUrl *a,
                           const EvaUrl *b)
{
  return a->scheme == b->scheme
      && strings_equal (a->host, b->host)
      && strings_equal (a->password, b->password)
      && eva_url_get_port (a) == eva_url_get_port (b)
      && strings_equal (a->user_name, b->user_name)
      && strings_equal (a->path, b->path)
      && strings_equal (a->query, b->query);
}

/**
 * eva_url_transfer_add_redirect:
 * @transfer: the Transfer to affect.
 * @request: request object for this segment of the transfer.
 * @response: response object for this segment of the transfer.
 * @is_permanent: whether the content is permanently relocated to this address.
 * @dest_url: the URL to which we have been redirected.
 *
 * Add an entry to the list of redirects
 * that we have encountered while trying to
 * service this request.
 *
 * Most users of EvaUrlTransfer won't care about these redirects--
 * they are provided to the rare client that cares about the redirect-path.
 * More commonly, users merely wish to suppress redirect handling: that can be done
 * more easily by eva_url_transfer_set_follow_redirects().
 *
 * This function should only be needed by implementors
 * of types of EvaUrlTransfer.
 *
 * returns: whether the redirect was allowed (it is disallowed if
 * it is a circular redirect. In that case, we will set 'transfer->error',
 * and call eva_url_transfer_notify_done().
 */
gboolean
eva_url_transfer_add_redirect    (EvaUrlTransfer     *transfer,
                                  GObject            *request,
                                  GObject            *response,
                                  gboolean            is_permanent,
                                  EvaUrl             *dest_url)
{
  EvaUrlTransferRedirect *redirect;
  g_return_val_if_fail (transfer->transfer_state == EVA_URL_TRANSFER_STATE_STARTED, TRUE);
  g_return_val_if_fail (EVA_IS_URL (dest_url), TRUE);

  /* Detect circular references. */
  if (urls_equal_up_to_fragment (dest_url, transfer->url))
    goto circular_redirect;
  for (redirect = transfer->first_redirect; redirect != NULL; redirect = redirect->next)
    if (urls_equal_up_to_fragment (redirect->url, dest_url))
      goto circular_redirect;

  redirect = g_new (EvaUrlTransferRedirect, 1);
  redirect->is_permanent = is_permanent;
  redirect->url = g_object_ref (dest_url);
  redirect->request = request ? g_object_ref (request) : transfer->request ? g_object_ref (transfer->request) : NULL;
  redirect->response = response ? g_object_ref (response) : NULL;
  redirect->next = NULL;

  if (transfer->first_redirect == NULL)
    transfer->first_redirect = redirect;
  else
    transfer->last_redirect->next = redirect;
  transfer->last_redirect = redirect;

  transfer->redirect_is_permanent = is_permanent;
  transfer->redirect_url = dest_url;
  return TRUE;


circular_redirect:
  eva_url_transfer_take_error (transfer,
                               g_error_new (EVA_G_ERROR_DOMAIN,
                                            EVA_ERROR_CIRCULAR,
                                            "circular redirects encountered"));
  eva_url_transfer_notify_done (transfer, EVA_URL_TRANSFER_ERROR_REDIRECT_LOOP);
  return FALSE;
}

/**
 * eva_url_transfer_set_download:
 * @transfer: the Transfer to affect.
 * @content: the content-stream for the downloaded data.
 *
 * Set the incoming-content that is associated with this transfer.
 * This will used by the user of the Transfer.
 *
 * This function should only be needed by implementors
 * of types of EvaUrlTransfer.
 */
void
eva_url_transfer_set_download    (EvaUrlTransfer     *transfer,
                                  EvaStream          *content)
{
  g_return_if_fail (transfer->transfer_state == EVA_URL_TRANSFER_STATE_STARTED);
  g_return_if_fail (transfer->content == NULL);
  g_return_if_fail (EVA_IS_STREAM (content));
  transfer->content = g_object_ref (content);
}

/**
 * eva_url_transfer_set_request:
 * @transfer: the Transfer to affect.
 * @request: the request object to store in the transfer information.
 *
 * Set the outgoing-request header data for this transaction.
 *
 * This function should only be needed by implementors
 * of types of EvaUrlTransfer.
 */
void
eva_url_transfer_set_request     (EvaUrlTransfer     *transfer,
                                  GObject            *request)
{
  GObject *old_request = transfer->request;
  g_return_if_fail (transfer->transfer_state == EVA_URL_TRANSFER_STATE_STARTED);
  g_return_if_fail (G_IS_OBJECT (request));
  transfer->request = g_object_ref (request);
  if (old_request)
    g_object_unref (old_request);
}

/**
 * eva_url_transfer_set_response:
 * @transfer: the Transfer to affect.
 * @response: the response object to store in the transfer information.
 *
 * Set the incoming-response header data for this transaction.
 *
 * This function should only be needed by implementors
 * of types of EvaUrlTransfer.
 */
void
eva_url_transfer_set_response    (EvaUrlTransfer     *transfer,
                                  GObject            *response)
{
  GObject *old_response = transfer->response;
  g_return_if_fail (transfer->transfer_state == EVA_URL_TRANSFER_STATE_STARTED);
  g_return_if_fail (transfer->response == NULL);
  transfer->response = g_object_ref (response);
  if (old_response)
    g_object_unref (old_response);
}

/**
 * eva_url_transfer_set_error:
 * @transfer: the Transfer to affect.
 * @error: the error to associate with the transfer.
 *
 * Set the error field for this transaction.
 * A copy of the error parameter is made.
 *
 * This function should only be needed by implementors
 * of types of EvaUrlTransfer.
 */
void
eva_url_transfer_set_error       (EvaUrlTransfer     *transfer,
                                  const GError       *error)
{
  GError *copy = g_error_copy (error);
  g_return_if_fail (error != NULL);
  if (transfer->error)
    g_error_free (transfer->error);
  transfer->error = copy;
}

/**
 * eva_url_transfer_take_error:
 * @transfer: the Transfer to affect.
 * @error: the error to associate with the transfer.
 *
 * Set the error field for this transaction.
 * The error parameter will be freed eventually by the
 * #EvaUrlTransfer.
 *
 * This function should only be needed by implementors
 * of types of EvaUrlTransfer.
 */
void
eva_url_transfer_take_error      (EvaUrlTransfer     *transfer,
                                  GError             *error)
{
  g_return_if_fail (error != NULL);
  if (error == transfer->error)
    return;
  if (transfer->error)
    g_error_free (transfer->error);
  transfer->error = error;
}

/**
 * eva_url_transfer_is_done:
 * @transfer: the Transfer to query.
 *
 * Find out whether the transfer is done.
 * The transfer is done iff the callback has been invoked.
 *
 * returns: whether the function is done.
 */
gboolean
eva_url_transfer_is_done (EvaUrlTransfer *transfer)
{
  return (transfer->transfer_state == EVA_URL_TRANSFER_STATE_DONE);
}

/**
 * eva_url_transfer_notify_done:
 * @transfer: the Transfer to affect.
 * @result: the transfer's result status code.
 *
 * Transition the transfer to the DONE state,
 * and invoke the user's callback (if any).
 * This function may only be invoked once per transfer.
 *
 * This function should only be needed by implementors
 * of types of EvaUrlTransfer.
 */
void
eva_url_transfer_notify_done     (EvaUrlTransfer     *transfer,
                                  EvaUrlTransferResult result)
{
  g_assert (transfer->transfer_state == EVA_URL_TRANSFER_STATE_STARTED);
  transfer->transfer_state = EVA_URL_TRANSFER_STATE_DONE;
  transfer->result = result;

  if (transfer->timeout_source)
    {
      EvaSource *timeout = transfer->timeout_source;
      transfer->timeout_source = NULL;
      eva_source_remove (timeout);
    }

  if (transfer->handler != NULL)
    transfer->handler (transfer, transfer->handler_data);

  /* we must relinquish these, or else there will circular ref-count leaks
     if the user tries to do tricks like the above commented code. */
  if (transfer->content != NULL)
    {
      EvaStream *tmp = transfer->content;
      transfer->content = NULL;
      g_object_unref (tmp);
    }
  if (transfer->upload_func != NULL)
    {
      gpointer data = transfer->upload_data;
      GDestroyNotify destroy = transfer->upload_destroy;
      transfer->upload_func = NULL;
      transfer->upload_data = NULL;
      transfer->upload_destroy = NULL;
      if (destroy)
        destroy (data);
    }

  if (transfer->handler_data_destroy)
    transfer->handler_data_destroy (transfer->handler_data);

  transfer->handler = NULL;
  transfer->handler_data_destroy = NULL;

  g_object_unref (transfer);
}

/* Registering a transfer type */
static GHashTable *scheme_to_slist_of_classes = NULL;
/**
 * eva_url_transfer_class_register:
 * @scheme: the URL scheme that this class of transfer can handle.
 * @transfer_class: the class that can handle the URL type.
 *
 * Register a class of URL transfer that can
 * handle a given scheme.
 * It will only be instantiated if the class' test method
 * returns TRUE, to indicate that it can handle the specific URL.
 */
void
eva_url_transfer_class_register  (EvaUrlScheme            scheme,
                                  EvaUrlTransferClass    *transfer_class)
{
  GSList *list;
  if (scheme_to_slist_of_classes == NULL)
    scheme_to_slist_of_classes = g_hash_table_new (NULL, NULL);
  list = g_hash_table_lookup (scheme_to_slist_of_classes, GUINT_TO_POINTER (scheme));
  if (list == NULL)
    {
      list = g_slist_prepend (NULL, transfer_class);
      g_hash_table_insert (scheme_to_slist_of_classes, GUINT_TO_POINTER (scheme), list);
    }
  else
    list = g_slist_append (list, transfer_class);
}

/**
 * eva_url_transfer_new:
 * @url: the URL to create a transfer object for.
 *
 * Create a URL transfer of the appropriate type for the given URL.
 * We try the registered classes, in order.
 *
 * returns: a newly allocated Transfer object, or NULL if no transfer-class
 * could handle the URL.
 */
EvaUrlTransfer *
eva_url_transfer_new             (EvaUrl             *url)
{
  GSList *list = g_hash_table_lookup (scheme_to_slist_of_classes, GUINT_TO_POINTER (url->scheme));
  while (list != NULL)
    {
      EvaUrlTransferClass *class = EVA_URL_TRANSFER_CLASS (list->data);
      if (class->test == NULL || class->test (class, url))
        {
          EvaUrlTransfer *transfer = g_object_new (G_OBJECT_CLASS_TYPE (class), NULL);
          eva_url_transfer_set_url (transfer, url);
          return transfer;
        }
      list = list->next;
    }
  return NULL;
}

/**
 * eva_url_transfer_get_state_string:
 * @transfer: the transfer to describe.
 *
 * Get a newly allocated, human-readable description
 * of the state of the transfer.
 *
 * returns: the newly-allocated string.
 */
char *
eva_url_transfer_get_state_string (EvaUrlTransfer *transfer)
{
  EvaUrlTransferClass *class = EVA_URL_TRANSFER_GET_CLASS (transfer);
  switch (transfer->transfer_state)
    {
    case EVA_URL_TRANSFER_STATE_CONSTRUCTING:
      return class->get_constructing_state (transfer);
    case EVA_URL_TRANSFER_STATE_STARTED:
      return class->get_running_state (transfer);
    case EVA_URL_TRANSFER_STATE_DONE:
      return class->get_done_state (transfer);
    default:
      return g_strdup ("eva_url_transfer_get_state_string: INVALID state");
    }
}


/* --- convert a transfer to a stream --- */
GType eva_url_transfer_stream_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_URL_TRANSFER_STREAM              (eva_url_transfer_stream_get_type ())
#define EVA_URL_TRANSFER_STREAM(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_URL_TRANSFER_STREAM, EvaUrlTransferStream))
#define EVA_URL_TRANSFER_STREAM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_URL_TRANSFER_STREAM, EvaUrlTransferStreamClass))
#define EVA_URL_TRANSFER_STREAM_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_URL_TRANSFER_STREAM, EvaUrlTransferStreamClass))
#define EVA_IS_URL_TRANSFER_STREAM(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_URL_TRANSFER_STREAM))
#define EVA_IS_URL_TRANSFER_STREAM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_URL_TRANSFER_STREAM))
typedef struct _UfUrlTransferStreamClass EvaUrlTransferStreamClass;
typedef struct _UfUrlTransferStream EvaUrlTransferStream;
struct _UfUrlTransferStreamClass
{
  EvaStreamClass base_class;
};
struct _UfUrlTransferStream
{
  EvaStream base_instance;
  EvaUrlTransfer *transfer;
  EvaStream *substream;
};

G_DEFINE_TYPE(EvaUrlTransferStream, eva_url_transfer_stream, EVA_TYPE_STREAM);

static void
eva_url_transfer_stream_finalize (GObject *object)
{
  EvaUrlTransferStream *transfer_stream = EVA_URL_TRANSFER_STREAM (object);
  g_assert (transfer_stream->transfer == NULL);
  if (transfer_stream->substream)
    {
      eva_io_untrap_readable (transfer_stream->substream);
      g_object_unref (transfer_stream->substream);
    }
  G_OBJECT_CLASS (eva_url_transfer_stream_parent_class)->finalize (object);
}
static gboolean
handle_substream_is_readable (EvaIO *io, gpointer data)
{
  eva_io_notify_ready_to_read (data);
  return TRUE;
}
static gboolean
handle_substream_read_shutdown (EvaIO *io, gpointer data)
{
  EvaUrlTransferStream *transfer_stream = EVA_URL_TRANSFER_STREAM (data);
  eva_io_notify_read_shutdown (EVA_IO (transfer_stream));
  if (transfer_stream->substream)
    {
      eva_io_untrap_readable (transfer_stream->substream);
      g_object_unref (transfer_stream->substream);
      transfer_stream->substream = NULL;
    }
  return FALSE;
}

static void
eva_url_transfer_stream_set_poll_read   (EvaIO      *io,
				        gboolean    do_poll)
{
  EvaUrlTransferStream *transfer_stream = EVA_URL_TRANSFER_STREAM (io);
  if (transfer_stream->substream == NULL)
    return;
  if (do_poll)
    eva_io_trap_readable (transfer_stream->substream,
                          handle_substream_is_readable,
                          handle_substream_read_shutdown,
                          transfer_stream,
                          NULL);
  else
    eva_io_untrap_readable (transfer_stream->substream);
}

static gboolean
eva_url_transfer_stream_shutdown_read   (EvaIO      *io,
                                        GError    **error)
{
  EvaUrlTransferStream *transfer_stream = EVA_URL_TRANSFER_STREAM (io);
  if (transfer_stream->transfer != NULL)
    eva_url_transfer_cancel (transfer_stream->transfer);
  if (transfer_stream->substream != NULL)
    eva_io_read_shutdown (EVA_IO (transfer_stream->substream), NULL);
  return TRUE;
}

static guint
eva_url_transfer_stream_raw_read (EvaStream     *stream,
			 	 gpointer       data,
			 	 guint          length,
			 	 GError       **error)
{
  EvaUrlTransferStream *transfer_stream = EVA_URL_TRANSFER_STREAM (stream);
  if (transfer_stream->substream == NULL)
    return 0;
  return eva_stream_read (transfer_stream->substream, data, length, error);
}

static guint
eva_url_transfer_stream_raw_read_buffer (EvaStream     *stream,
				        EvaBuffer     *buffer,
				        GError       **error)
{
  EvaUrlTransferStream *transfer_stream = EVA_URL_TRANSFER_STREAM (stream);
  if (transfer_stream->substream == NULL)
    return 0;
  return eva_stream_read_buffer (transfer_stream->substream, buffer, error);
}

static void
eva_url_transfer_stream_class_init (EvaUrlTransferStreamClass *class)
{
  EvaIOClass *io_class = EVA_IO_CLASS (class);
  EvaStreamClass *stream_class = EVA_STREAM_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  stream_class->raw_read = eva_url_transfer_stream_raw_read;
  stream_class->raw_read_buffer = eva_url_transfer_stream_raw_read_buffer;
  io_class->set_poll_read = eva_url_transfer_stream_set_poll_read;
  io_class->shutdown_read = eva_url_transfer_stream_shutdown_read;
  object_class->finalize = eva_url_transfer_stream_finalize;
}

static void
eva_url_transfer_stream_init (EvaUrlTransferStream *transfer_stream)
{
  eva_stream_mark_is_readable (transfer_stream);
}

static void
handle_transfer_done (EvaUrlTransfer *transfer,
                      gpointer        data)
{
  EvaUrlTransferStream *transfer_stream = EVA_URL_TRANSFER_STREAM (data);
  g_assert (transfer_stream->transfer == transfer);
  transfer_stream->transfer = NULL;

  if (transfer->error != NULL)
    eva_io_set_gerror (EVA_IO (transfer_stream), EVA_IO_ERROR_CONNECT,
                       g_error_copy (transfer->error));
  if (transfer->content != NULL)
    {
      transfer_stream->substream = g_object_ref (transfer->content);
      if (eva_io_is_polling_for_read (transfer_stream))
        eva_io_trap_readable (transfer_stream->substream,
                              handle_substream_is_readable,
                              handle_substream_read_shutdown,
                              g_object_ref (transfer_stream),
                              g_object_unref);
    }
  else
    {
      eva_io_notify_read_shutdown (EVA_IO (transfer_stream));
    }
}

/**
 * eva_url_transfer_stream_new:
 * @transfer: the transfer.  must not be started.
 * @error: optional location to store the #GError if there is a problem.
 *
 * This code will start the transfer,
 * and return a stream that you can trap immediately.
 *
 * returns: the new stream, or NULL if an error occurred.
 */
EvaStream *
eva_url_transfer_stream_new (EvaUrlTransfer *transfer,
                             GError        **error)
{
  EvaUrlTransferStream *transfer_stream = g_object_new (EVA_TYPE_URL_TRANSFER_STREAM, NULL);
  transfer_stream->transfer = transfer;
  eva_url_transfer_set_handler (transfer,
                                handle_transfer_done,
                                g_object_ref (transfer_stream),
                                g_object_unref);
  if (!eva_url_transfer_start (transfer, error))
    {
      transfer_stream->transfer = NULL;
      g_object_unref (transfer_stream);
      return NULL;
    }
  return EVA_STREAM (transfer_stream);
}
