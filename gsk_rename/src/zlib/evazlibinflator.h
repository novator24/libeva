#ifndef __EVA_ZLIB_INFLATOR_H_
#define __EVA_ZLIB_INFLATOR_H_

#include "../evastream.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _EvaZlibInflator EvaZlibInflator;
typedef struct _EvaZlibInflatorClass EvaZlibInflatorClass;
/* --- type macros --- */
GType eva_zlib_inflator_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_ZLIB_INFLATOR			(eva_zlib_inflator_get_type ())
#define EVA_ZLIB_INFLATOR(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_ZLIB_INFLATOR, EvaZlibInflator))
#define EVA_ZLIB_INFLATOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_ZLIB_INFLATOR, EvaZlibInflatorClass))
#define EVA_ZLIB_INFLATOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_ZLIB_INFLATOR, EvaZlibInflatorClass))
#define EVA_IS_ZLIB_INFLATOR(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_ZLIB_INFLATOR))
#define EVA_IS_ZLIB_INFLATOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_ZLIB_INFLATOR))

/* --- structures --- */
struct _EvaZlibInflatorClass 
{
  EvaStreamClass stream_class;
};
struct _EvaZlibInflator 
{
  EvaStream      stream;
  gpointer       private_stream;
  EvaBuffer      decompressed;
  gboolean       use_gzip;
};

/* --- prototypes --- */
EvaStream *eva_zlib_inflator_new (void);
EvaStream *eva_zlib_inflator_new2 (gboolean use_gzip);

G_END_DECLS

#endif
