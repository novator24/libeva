#include "evaurltransferfile.h"
#include "../evastreamfd.h"

G_DEFINE_TYPE(EvaUrlTransferFile, eva_url_transfer_file, EVA_TYPE_URL_TRANSFER);

static gboolean 
eva_url_transfer_file_test  (EvaUrlTransferClass *transfer_class,
                             const EvaUrl        *url)
{
  return url->scheme == EVA_URL_SCHEME_FILE
    && url->path != NULL
    && url->path[0] == '/';
}

static gboolean 
eva_url_transfer_file_start (EvaUrlTransfer      *transfer,
                             GError             **error)
{
  EvaStream *content;
  content = eva_stream_fd_new_read_file (transfer->url->path, error);
  if (content == NULL)
    return FALSE;
  eva_url_transfer_set_download (transfer, content);
  g_object_unref (content);
  eva_url_transfer_notify_done (transfer, EVA_URL_TRANSFER_SUCCESS);
  return TRUE;
}

static void
eva_url_transfer_file_class_init (EvaUrlTransferFileClass *class)
{
  EvaUrlTransferClass *transfer_class = EVA_URL_TRANSFER_CLASS (class);
  transfer_class->test = eva_url_transfer_file_test;
  transfer_class->start = eva_url_transfer_file_start;
}
static void
eva_url_transfer_file_init (EvaUrlTransferFile *file)
{
}
