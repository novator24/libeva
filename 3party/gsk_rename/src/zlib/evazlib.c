#include "evazlib.h"
#include <zlib.h>

/**
 * eva_zlib_error_to_eva_error:
 * @zlib_error_rv: error code returned by zlib.
 * These are Z_OK, Z_STREAM_END, Z_NEED_DICT,
 * Z_ERRNO, Z_STREAM_ERROR, Z_DATA_ERROR, etc.
 * 
 * Converts a zlib error code to a #EvaErrorCode.
 *
 * returns: the best approximation for the zlib error
 * in EVA's list.
 */
EvaErrorCode eva_zlib_error_to_eva_error(gint zlib_error_rv)
{
  switch (zlib_error_rv)
    {
    case Z_OK: return EVA_ERROR_NONE;
    case Z_STREAM_END: return EVA_ERROR_END_OF_FILE;
    case Z_NEED_DICT: return EVA_ERROR_NOT_READY;
    case Z_ERRNO: return EVA_ERROR_UNKNOWN;
    case Z_STREAM_ERROR: return EVA_ERROR_IO;
    case Z_DATA_ERROR: return EVA_ERROR_BAD_FORMAT;
    case Z_MEM_ERROR: return EVA_ERROR_OUT_OF_MEMORY;
    case Z_BUF_ERROR: return EVA_ERROR_FULL;
    case Z_VERSION_ERROR: return EVA_ERROR_VERSION;
    default: return EVA_ERROR_UNKNOWN;
    }
}

/**
 * eva_zlib_error_to_message:
 * @zlib_error_rv: the error code to convert to a string.
 *
 * Find a string which corresponds to the given zlib error code.
 *
 * returns: the error message string text.
 */
const char * eva_zlib_error_to_message  (gint zlib_error_rv)
{
#ifdef EVA_DEBUG
#define SUFFIX(p) " (" #p ")"
#else
#define SUFFIX(p)
#endif
  switch (zlib_error_rv)
    {
    case Z_OK: return "No error" SUFFIX(Z_OK);
    case Z_STREAM_END: return "End-of-data" SUFFIX(Z_STREAM_END);
    case Z_NEED_DICT: return "Need dictionary" SUFFIX(Z_NEED_DICT);
    case Z_ERRNO: return "See errno" SUFFIX(Z_ERRNO);
    case Z_STREAM_ERROR: return "Stream error" SUFFIX(Z_STREAM_ERROR);
    case Z_DATA_ERROR: return "Data error" SUFFIX(Z_DATA_ERROR);
    case Z_MEM_ERROR: return "Out-of-memory" SUFFIX(Z_MEM_ERROR);
    case Z_BUF_ERROR: return "Buffer full" SUFFIX(Z_BUF_ERROR);
    case Z_VERSION_ERROR: return "Version mismatch" SUFFIX(Z_VERSION_ERROR);
    default: return "Unknown Error!!!";
    }
#undef SUFFIX
}
