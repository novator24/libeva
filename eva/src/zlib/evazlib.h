#ifndef __EVA_ZLIB_COMMON_H_
#define __EVA_ZLIB_COMMON_H_

#include "../evaerror.h"

G_BEGIN_DECLS

GskErrorCode eva_zlib_error_to_eva_error(gint zlib_error_rv);
const char * eva_zlib_error_to_message  (gint zlib_error_rv);

G_END_DECLS

#endif
