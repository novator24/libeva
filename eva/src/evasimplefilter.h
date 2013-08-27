#ifndef __EVA_SIMPLE_FILTER_H_
#define __EVA_SIMPLE_FILTER_H_

#include "evastream.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _EvaSimpleFilter EvaSimpleFilter;
typedef struct _EvaSimpleFilterClass EvaSimpleFilterClass;
/* --- type macros --- */
GType eva_simple_filter_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_SIMPLE_FILTER			(eva_simple_filter_get_type ())
#define EVA_SIMPLE_FILTER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_SIMPLE_FILTER, EvaSimpleFilter))
#define EVA_SIMPLE_FILTER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_SIMPLE_FILTER, EvaSimpleFilterClass))
#define EVA_SIMPLE_FILTER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_SIMPLE_FILTER, EvaSimpleFilterClass))
#define EVA_IS_SIMPLE_FILTER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_SIMPLE_FILTER))
#define EVA_IS_SIMPLE_FILTER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_SIMPLE_FILTER))

/* --- structures --- */
struct _EvaSimpleFilterClass 
{
  EvaStreamClass stream_class;
  gboolean (*process)(EvaSimpleFilter *filter,
		      EvaBuffer       *dst,
		      EvaBuffer       *src,
		      GError         **error);
  gboolean (*flush)  (EvaSimpleFilter *filter,
		      EvaBuffer       *dst,
		      EvaBuffer       *src,
		      GError         **error);
};
struct _EvaSimpleFilter 
{
  EvaStream      stream;

  /*< private >*/
  EvaBuffer read_buffer;
  EvaBuffer write_buffer;
  guint     max_read_buffer_size;
  guint     max_write_buffer_size;
};
/* --- prototypes --- */
G_END_DECLS

#endif
