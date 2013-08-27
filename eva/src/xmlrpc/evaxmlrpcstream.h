#ifndef __EVA_XMLRPC_STREAM_H_
#define __EVA_XMLRPC_STREAM_H_

#include "evaxmlrpc.h"
#include "../evastream.h"

G_BEGIN_DECLS

/*Defined in evaxmlrpc.h */
/* --- typedefs --- */
/* typedef struct _EvaXmlrpcStream EvaXmlrpcStream; */
/* typedef struct _EvaXmlrpcStreamClass EvaXmlrpcStreamClass; */
/* typedef struct _EvaXmlrpcOutgoing EvaXmlrpcOutgoing; */
/* typedef struct _EvaXmlrpcIncoming EvaXmlrpcIncoming; */

/* --- type macros --- */
GType eva_xmlrpc_stream_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_XMLRPC_STREAM			(eva_xmlrpc_stream_get_type ())
#define EVA_XMLRPC_STREAM(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_XMLRPC_STREAM, EvaXmlrpcStream))
#define EVA_XMLRPC_STREAM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_XMLRPC_STREAM, EvaXmlrpcStreamClass))
#define EVA_XMLRPC_STREAM_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_XMLRPC_STREAM, EvaXmlrpcStreamClass))
#define EVA_IS_XMLRPC_STREAM(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_XMLRPC_STREAM))
#define EVA_IS_XMLRPC_STREAM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_XMLRPC_STREAM))

/* --- structures --- */
struct _EvaXmlrpcStreamClass 
{
  EvaStreamClass stream_class;

  void (*set_poll_requestable) (EvaXmlrpcStream *,
				gboolean polling);
  void (*shutdown_requestable) (EvaXmlrpcStream *);
};

struct _EvaXmlrpcStream 
{
  EvaStream      stream;
  EvaXmlrpcParser *parser;

  /* handle incoming requests */
  EvaXmlrpcIncoming *first_unhandled_request;
  EvaXmlrpcIncoming *next_to_dequeue;
  EvaXmlrpcIncoming *last_request;
  EvaHook incoming_request_hook;

  /* handle outgoing requests */
  EvaXmlrpcOutgoing *first_unresponded_request;
  EvaXmlrpcOutgoing *last_unresponded_request;

  /* queue outgoing response and request data here */
  EvaBuffer outgoing;
};


/* --- prototypes --- */

/* trap this hook to get notification when a request is available. */
#define EVA_XMLRPC_STREAM_REQUEST_HOOK(stream) (&(EVA_XMLRPC_STREAM (stream)->incoming_request_hook))

#define eva_xmlrpc_stream_request_trap(server,func,shutdown,data,destroy) \
                               eva_hook_trap (EVA_XMLRPC_STREAM_REQUEST_HOOK (server), (EvaHookFunc) func,    \
                               (EvaHookFunc) shutdown, data, destroy)

#define eva_xmlrpc_stream_request_untrap(server) eva_hook_untrap (EVA_XMLRPC_STREAM_REQUEST_HOOK (server))

/* Handle incoming requests. */
EvaXmlrpcRequest *eva_xmlrpc_stream_get_request (EvaXmlrpcStream *stream);
void              eva_xmlrpc_stream_respond     (EvaXmlrpcStream *stream,
						 EvaXmlrpcRequest *request,
						 EvaXmlrpcResponse *response);

/* Make outgoing requests. */
typedef void (*EvaXmlrpcResponseNotify) (EvaXmlrpcRequest  *request,
					 EvaXmlrpcResponse *response,
					 gpointer           data);
void              eva_xmlrpc_stream_make_request (EvaXmlrpcStream *stream,
						  EvaXmlrpcRequest *request,
						  EvaXmlrpcResponseNotify notify,
						  gpointer data,
						  GDestroyNotify destroy);

G_END_DECLS

#endif
