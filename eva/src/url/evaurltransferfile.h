#ifndef __EVA_URL_TRANSFER_FILE_H_
#define __EVA_URL_TRANSFER_FILE_H_

#include "evaurltransfer.h"

G_BEGIN_DECLS

typedef struct _GskUrlTransferFileClass GskUrlTransferFileClass;
typedef struct _GskUrlTransferFile GskUrlTransferFile;

GType eva_url_transfer_file_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_URL_TRANSFER_FILE              (eva_url_transfer_file_get_type ())
#define EVA_URL_TRANSFER_FILE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_URL_TRANSFER_FILE, GskUrlTransferFile))
#define EVA_URL_TRANSFER_FILE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_URL_TRANSFER_FILE, GskUrlTransferFileClass))
#define EVA_URL_TRANSFER_FILE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_URL_TRANSFER_FILE, GskUrlTransferFileClass))
#define EVA_IS_URL_TRANSFER_FILE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_URL_TRANSFER_FILE))
#define EVA_IS_URL_TRANSFER_FILE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_URL_TRANSFER_FILE))

struct _GskUrlTransferFileClass
{
  GskUrlTransferClass base_class;
};

struct _GskUrlTransferFile
{
  GskUrlTransfer base_instance;
};

G_END_DECLS

#endif
