#ifndef __EVA_ZLIB_DEFLATOR_H_
#define __EVA_ZLIB_DEFLATOR_H_

#include "../evastream.h"
#include "../evamainloop.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _GskZlibDeflator GskZlibDeflator;
typedef struct _GskZlibDeflatorClass GskZlibDeflatorClass;
/* --- type macros --- */
GType eva_zlib_deflator_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_ZLIB_DEFLATOR			(eva_zlib_deflator_get_type ())
#define EVA_ZLIB_DEFLATOR(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_ZLIB_DEFLATOR, GskZlibDeflator))
#define EVA_ZLIB_DEFLATOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_ZLIB_DEFLATOR, GskZlibDeflatorClass))
#define EVA_ZLIB_DEFLATOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_ZLIB_DEFLATOR, GskZlibDeflatorClass))
#define EVA_IS_ZLIB_DEFLATOR(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_ZLIB_DEFLATOR))
#define EVA_IS_ZLIB_DEFLATOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_ZLIB_DEFLATOR))

/* --- structures --- */
struct _GskZlibDeflatorClass 
{
  GskStreamClass stream_class;
};
struct _GskZlibDeflator 
{
  GskStream      stream;
  gpointer       private_stream;
  GskBuffer      compressed;
  guint          level;
  gint           flush_millis;
  GskSource     *flush_source;
  gboolean       use_gzip;
};

/* --- prototypes --- */
/* set to -1 for default compression level;
   otherwise use 0..9, like the arguments to gzip. */
GskStream *eva_zlib_deflator_new (gint compression_level,
                                  int flush_millis);

GskStream *eva_zlib_deflator_new2 (int compression_level,
                                   int flush_millis,
                                   gboolean use_gzip);
G_END_DECLS

#endif
