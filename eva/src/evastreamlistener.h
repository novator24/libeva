#ifndef __EVA_STREAM_LISTENER_H_
#define __EVA_STREAM_LISTENER_H_

#include "evastream.h"

G_BEGIN_DECLS

typedef struct _GskStreamListener GskStreamListener;
typedef struct _GskStreamListenerClass GskStreamListenerClass;

GType eva_stream_listener_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_STREAM_LISTENER	      (eva_stream_listener_get_type ())
#define EVA_STREAM_LISTENER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_STREAM_LISTENER, GskStreamListener))
#define EVA_STREAM_LISTENER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_STREAM_LISTENER, GskStreamListenerClass))
#define EVA_STREAM_LISTENER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_STREAM_LISTENER, GskStreamListenerClass))
#define EVA_IS_STREAM_LISTENER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_STREAM_LISTENER))
#define EVA_IS_STREAM_LISTENER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_STREAM_LISTENER))

/* try and handle a newly opened stream socket (is_open) */
typedef gboolean (*GskStreamListenerAcceptFunc)(GskStream    *stream,
						gpointer      data,
						GError      **error);

/* handle an error accepting the latest connection */
typedef void     (*GskStreamListenerErrorFunc) (GError       *err,
						gpointer      data);

/* --- structures --- */
struct _GskStreamListenerClass 
{
  GObjectClass object_class;
};
struct _GskStreamListener 
{
  GObject      object;

  GskStreamListenerAcceptFunc accept_func;
  GskStreamListenerErrorFunc error_func;
  gpointer           data;
  GDestroyNotify     destroy;

  GError  *last_error;
};

/* --- prototypes --- */
void eva_stream_listener_handle_accept   (GskStreamListener *listener,
				          GskStreamListenerAcceptFunc func,
					  GskStreamListenerErrorFunc err_func,
					  gpointer           data,
					  GDestroyNotify     destroy);

/* protected */
void eva_stream_listener_notify_accepted (GskStreamListener *stream_listener,
					  GskStream         *new_stream);
void eva_stream_listener_notify_error    (GskStreamListener *stream_listener,
					  GError            *error);

G_END_DECLS

#endif
