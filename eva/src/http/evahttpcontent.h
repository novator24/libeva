#ifndef __EVA_HTTP_CONTENT_H_
#define __EVA_HTTP_CONTENT_H_

#include "evahttpserver.h"
#include "../mime/evamimemultipartpiece.h"
#include "../evasocketaddress.h"

G_BEGIN_DECLS

/* TODO:
     - provide error page handler functions (eg something
       which takes a server,request,response_status,error_message
       and responds to the user with it.  one variant
       registered with a particular error code,
       one that handles any error code.
       (provide suitable defaults)

     - likewise, redirect handlers.

     - ...

     - make a EvaTableTree type for managing hierarchical data 
       like this, to clean it up (pretty hard!)
 */

/* class to help run an http server */

/* opaque class */
typedef struct _EvaHttpContentHandler EvaHttpContentHandler;
typedef struct _EvaHttpContent EvaHttpContent;
typedef struct _EvaHttpContentId EvaHttpContentId;

typedef enum
{
  EVA_HTTP_CONTENT_OK,
  EVA_HTTP_CONTENT_CHAIN,
  EVA_HTTP_CONTENT_ERROR        /* causes a 500 error to result */
} EvaHttpContentResult;

typedef EvaHttpContentResult (*EvaHttpContentFunc)    (EvaHttpContent   *content,
                                                       EvaHttpContentHandler *handler,
                                                       EvaHttpServer  *server,
                                                       EvaHttpRequest *request,
                                                       EvaStream      *post_data,
                                                       gpointer        data);


/* note: CGIs cannot chain */
typedef void                 (*EvaHttpContentCGIFunc) (EvaHttpContent   *content,
                                                       EvaHttpContentHandler *handler,
                                                       EvaHttpServer  *server,
                                                       EvaHttpRequest *request,
					               guint           n_vars,
					               EvaMimeMultipartPiece **vars,
                                                       gpointer        data);


struct _EvaHttpContentId
{
  const char *host;
  const char *user_agent_prefix;
  const char *path;
  const char *path_prefix;
  const char *path_suffix;
};
#define EVA_HTTP_CONTENT_ID_INIT {NULL,NULL,NULL,NULL,NULL}

/* the order hook tables are tried is:
      with user_agent, with path and vhost
      with user_agent, with vhost
      with user_agent, with path
      with user_agent, no path or host
      without user_agent, with path and vhost
      without user_agent, with vhost
      without user_agent, with path
      without user_agent, no path or host */

EvaHttpContent *eva_http_content_new     (void);
EvaHttpContentHandler *
eva_http_content_handler_new     (EvaHttpContentFunc    func,
                                  gpointer              data,
                                  GDestroyNotify        destroy);

EvaHttpContentHandler *
eva_http_content_handler_new_cgi (EvaHttpContentCGIFunc func,
                                  gpointer              data,
                                  GDestroyNotify        destroy);

void eva_http_content_handler_ref  (EvaHttpContentHandler *handler);
void eva_http_content_handler_unref(EvaHttpContentHandler *handler);


typedef enum
{
  EVA_HTTP_CONTENT_PREPEND,
  EVA_HTTP_CONTENT_APPEND,
  EVA_HTTP_CONTENT_REPLACE
} EvaHttpContentAction;

void           eva_http_content_add_handler  (EvaHttpContent          *content,
                                              const EvaHttpContentId  *id,
                                              EvaHttpContentHandler   *handler,
                                              EvaHttpContentAction     action);

void           eva_http_content_set_mime_type(EvaHttpContent *content,
                                              const char     *prefix,
                                              const char     *suffix,
					      const char     *type,
					      const char     *subtype);
void           eva_http_content_set_default_mime_type
                                             (EvaHttpContent *content,
					      const char     *type,
					      const char     *subtype);
gboolean       eva_http_content_get_mime_type(EvaHttpContent *content,
                                              const char     *path,
					      const char    **type_out,
					      const char    **subtype_out);
void       eva_http_content_set_idle_timeout (EvaHttpContent *content);

typedef void (*EvaHttpContentErrorHandler)(EvaHttpContent          *content,
                                           GError                  *error,
                                           EvaHttpServer           *server,
                                           EvaHttpRequest          *request,
                                           EvaHttpStatus            code,
                                           gpointer                 data);

void eva_http_content_set_error_handler  (EvaHttpContent          *content,
                                          EvaHttpContentErrorHandler handler,
                                          gpointer                 data,
                                          GDestroyNotify           destroy);

/* helpers */
void           eva_http_content_add_data (EvaHttpContent          *content,
                                          const EvaHttpContentId  *id,
                                          gconstpointer            data,
					  guint                    data_len,
					  gpointer                 destroy_data,
				          GDestroyNotify           destroy);
void           eva_http_content_add_data_by_path
                                         (EvaHttpContent          *content,
                                          const char              *path,
                                          gconstpointer            data,
					  guint                    data_len,
					  gpointer                 destroy_data,
				          GDestroyNotify           destroy);
typedef enum
{
  EVA_HTTP_CONTENT_FILE_EXACT,
  EVA_HTTP_CONTENT_FILE_DIR,
  EVA_HTTP_CONTENT_FILE_DIR_TREE
} EvaHttpContentFileType;
void           eva_http_content_add_file (EvaHttpContent          *content,
                                          const char              *path,
					  const char              *fs_path,
					  EvaHttpContentFileType   type);

/* note: id must include a path_prefix */
void           eva_http_content_add_file_by_id
                                         (EvaHttpContent          *content,
                                          const EvaHttpContentId  *id,
					  const char              *fs_path,
					  EvaHttpContentFileType   type);


/* --- serving pages --- */
gboolean eva_http_content_listen (EvaHttpContent *content,
                                  EvaSocketAddress *address,
                                  GError          **error);
void eva_http_content_respond    (EvaHttpContent *content,
                                  EvaHttpServer  *server,
                                  EvaHttpRequest *request,
			          EvaStream      *post_data);
void eva_http_content_manage_server (EvaHttpContent *content,
                                     EvaHttpServer  *server);

G_END_DECLS

#endif
