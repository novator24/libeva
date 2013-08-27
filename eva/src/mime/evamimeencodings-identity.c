#include "evamimeencodings.h"
#include "../evasimplefilter.h"

typedef struct _GskMimeIdentityFilter GskMimeIdentityFilter;
typedef struct _GskMimeIdentityFilterClass GskMimeIdentityFilterClass;
GType eva_mime_identity_filter_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_MIME_IDENTITY_FILTER			(eva_mime_identity_filter_get_type ())
#define EVA_MIME_IDENTITY_FILTER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_MIME_IDENTITY_FILTER, GskMimeIdentityFilter))
#define EVA_MIME_IDENTITY_FILTER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_MIME_IDENTITY_FILTER, GskMimeIdentityFilterClass))
#define EVA_MIME_IDENTITY_FILTER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_MIME_IDENTITY_FILTER, GskMimeIdentityFilterClass))
#define EVA_IS_MIME_IDENTITY_FILTER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_MIME_IDENTITY_FILTER))
#define EVA_IS_MIME_IDENTITY_FILTER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_MIME_IDENTITY_FILTER))

struct _GskMimeIdentityFilterClass 
{
  GskSimpleFilterClass simple_filter_class;
};
struct _GskMimeIdentityFilter 
{
  GskSimpleFilter      simple_filter;
};
static GObjectClass *parent_class = NULL;

static gboolean
eva_mime_identity_filter_process (GskSimpleFilter *filter,
                                  GskBuffer       *dst,
                                  GskBuffer       *src,
                                  GError         **error)
{
  eva_buffer_drain (dst, src);
  return TRUE;
}

static gboolean
eva_mime_identity_filter_flush (GskSimpleFilter *filter,
                                GskBuffer       *dst,
                                GskBuffer       *src,
                                GError         **error)
{
  g_return_val_if_fail (src->size == 0, FALSE);
  return TRUE;
}

/* --- functions --- */
static void
eva_mime_identity_filter_init (GskMimeIdentityFilter *mime_identity_filter)
{
  g_assert (eva_io_get_is_writable (mime_identity_filter));
  g_assert (eva_io_get_is_readable (mime_identity_filter));
}
static void
eva_mime_identity_filter_class_init (GskMimeIdentityFilterClass *class)
{
  GskSimpleFilterClass *simple_filter_class = EVA_SIMPLE_FILTER_CLASS (class);
  parent_class = g_type_class_peek_parent (class);
  simple_filter_class->process = eva_mime_identity_filter_process;
  simple_filter_class->flush = eva_mime_identity_filter_flush;
}

GType eva_mime_identity_filter_get_type()
{
  static GType mime_identity_filter_type = 0;
  if (!mime_identity_filter_type)
    {
      static const GTypeInfo mime_identity_filter_info =
      {
	sizeof(GskMimeIdentityFilterClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_mime_identity_filter_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GskMimeIdentityFilter),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_mime_identity_filter_init,
	NULL		/* value_table */
      };
      mime_identity_filter_type = g_type_register_static (EVA_TYPE_SIMPLE_FILTER,
                                                  "GskMimeIdentityFilter",
						  &mime_identity_filter_info, 0);
    }
  return mime_identity_filter_type;
}

/**
 * eva_mime_identity_filter_new:
 *
 * A filter which gives the exact same output as it receives input.
 *
 * returns: the newly allocated identity filter.
 */
GskStream *
eva_mime_identity_filter_new (void)
{
  return g_object_new (EVA_TYPE_MIME_IDENTITY_FILTER, NULL);
}
