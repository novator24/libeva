/* This function is called by eva_init() */
#include "evaurltransferhttp.h"
#include "evaurltransferfile.h"

void
_eva_url_transfer_register_builtins (void)
{
  GskUrlTransferClass *class;

  class = g_type_class_ref (EVA_TYPE_URL_TRANSFER_HTTP);
  eva_url_transfer_class_register (EVA_URL_SCHEME_HTTP, class);
  eva_url_transfer_class_register (EVA_URL_SCHEME_HTTPS, class);

  class = g_type_class_ref (EVA_TYPE_URL_TRANSFER_FILE);
  eva_url_transfer_class_register (EVA_URL_SCHEME_FILE, class);
}
