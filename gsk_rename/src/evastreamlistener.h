#ifndef __EVA_STREAM_LISTENER_H_
#define __EVA_STREAM_LISTENER_H_

#include "evastream.h"

G_BEGIN_DECLS

typedef struct _EvaStreamListener EvaStreamListener;
typedef struct _EvaStreamListenerClass EvaStreamListenerClass;

GType eva_stream_listener_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_STREAM_LISTENER	      (eva_stream_listener_get_type ())
#define EVA_STREAM_LISTENER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_STREAM_LISTENER, EvaStreamListener))
#define EVA_STREAM_LISTENER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_STREAM_LISTENER, EvaStreamListenerClass))
#define EVA_STREAM_LISTENER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_STREAM_LISTENER, EvaStreamListenerClass))
#define EVA_IS_STREAM_LISTENER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_STREAM_LISTENER))
#define EVA_IS_STREAM_LISTENER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_STREAM_LISTENER))

/* try and handle a newly opened stream socket (is_open) */
typedef gboolean (*EvaStreamListenerAcceptFunc)(EvaStream    *stream,
						gpointer      data,
						GError      **error);

/* handle an error accepting the latest connection */
typedef void     (*EvaStreamListenerErrorFunc) (GError       *err,
						gpointer      data);

/* --- structures --- */
struct _EvaStreamListenerClass 
{
  GObjectClass object_class;
};
struct _EvaStreamListener 
{
  GObject      object;

  EvaStreamListenerAcceptFunc accept_func;
  EvaStreamListenerErrorFunc error_func;
  gpointer           data;
  GDestroyNotify     destroy;

  GError  *last_error;
};

/* --- prototypes --- */
void eva_stream_listener_handle_accept   (EvaStreamListener *listener,
				          EvaStreamListenerAcceptFunc func,
					  EvaStreamListenerErrorFunc err_func,
					  gpointer           data,
					  GDestroyNotify     destroy);

/* protected */
void eva_stream_listener_notify_accepted (EvaStreamListener *stream_listener,
					  EvaStream         *new_stream);
void eva_stream_listener_notify_error    (EvaStreamListener *stream_listener,
					  GError            *error);

G_END_DECLS

#endif
