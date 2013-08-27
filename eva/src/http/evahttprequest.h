#ifndef __EVA_HTTP_REQUEST_H_
#define __EVA_HTTP_REQUEST_H_

#ifndef __EVA_HTTP_HEADER_H_
#include "evahttpheader.h"
#endif

G_BEGIN_DECLS

typedef struct _EvaHttpRequestClass EvaHttpRequestClass;
typedef struct _EvaHttpRequest EvaHttpRequest;

#define EVA_TYPE_HTTP_REQUEST              (eva_http_request_get_type ())
#define EVA_HTTP_REQUEST(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_HTTP_REQUEST, EvaHttpRequest))
#define EVA_HTTP_REQUEST_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_HTTP_REQUEST, EvaHttpRequestClass))
#define EVA_HTTP_REQUEST_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_HTTP_REQUEST, EvaHttpRequestClass))
#define EVA_IS_HTTP_REQUEST(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_HTTP_REQUEST))
#define EVA_IS_HTTP_REQUEST_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_HTTP_REQUEST))

struct _EvaHttpRequestClass
{
  EvaHttpHeaderClass base_class;
};
struct _EvaHttpRequest
{
  EvaHttpHeader base_instance;
  
  /*< public >*/
  /* the command: GET, PUT, POST, HEAD, etc */
  EvaHttpVerb                   verb;

  /* Note that HTTP/1.1 servers must accept the entire
   * URL being included in `path'! (maybe including http:// ... */
  char                         *path;

  EvaHttpCharSet           *accept_charsets;              /* Accept-CharSet */
  EvaHttpContentEncodingSet*accept_content_encodings;     /* Accept-Encoding */
  EvaHttpTransferEncodingSet*accept_transfer_encodings;   /* TE */
  EvaHttpMediaTypeSet      *accept_media_types;           /* Accept */
  EvaHttpAuthorization     *authorization;                /* Authorization */
  EvaHttpLanguageSet       *accept_languages;             /* Accept-Languages */
  char                     *host;                         /* Host */

  gboolean                  had_if_match;
  char                    **if_match;             /* If-Match */
  glong                     if_modified_since;    /* If-Modified-Since */
  char                     *user_agent;           /* User-Agent */

  char                     *referrer;             /* Referer */

  char                     *from;      /* The From: header (sect 14.22) */

  /* List of Cookie: headers. */
  GSList                   *cookies;

  EvaHttpAuthorization     *proxy_authorization;

  int                       keep_alive_seconds;   /* -1 if not used */

  /* rarely used: */
  int                       max_forwards;         /* -1 if not used */

  /* Nonstandard User-Agent information.
     Many browsers provide this data to allow
     dynamic content to take advantage of the
     client configuration.  (0 indicated "not supplied").  */
  unsigned                  ua_width, ua_height;
  char                     *ua_color;
  char                     *ua_os;
  char                     *ua_cpu;
  char                     *ua_language;

  EvaHttpRequestCacheDirective *cache_control;        /* Cache-Control */
};

/* --- cookies --- */
void            eva_http_request_add_cookie      (EvaHttpRequest *header,
                                                  EvaHttpCookie  *cookie);
void            eva_http_request_remove_cookie   (EvaHttpRequest *header,
                                                  EvaHttpCookie  *cookie);
EvaHttpCookie  *eva_http_request_find_cookie     (EvaHttpRequest *header,
                                                  const char     *key);

void eva_http_request_set_cache_control (EvaHttpRequest               *request,
                                         EvaHttpRequestCacheDirective *directive);

gboolean        eva_http_request_has_content_body (EvaHttpRequest *request);

/* request specific functions */
/* unhandled: if_match */
void            eva_http_request_add_charsets            (EvaHttpRequest *header,
                                                          EvaHttpCharSet *char_sets);
void            eva_http_request_clear_charsets          (EvaHttpRequest *header);
void            eva_http_request_add_content_encodings   (EvaHttpRequest *header,
                                                          EvaHttpContentEncodingSet *set);
void            eva_http_request_clear_content_encodings (EvaHttpRequest *header);
void            eva_http_request_add_transfer_encodings  (EvaHttpRequest *header,
                                                          EvaHttpTransferEncodingSet *set);
void            eva_http_request_clear_transfer_encodings(EvaHttpRequest *header);
void            eva_http_request_add_media               (EvaHttpRequest *header,
                                                          EvaHttpMediaTypeSet *set);
void            eva_http_request_clear_media             (EvaHttpRequest *header);
void            eva_http_request_set_authorization       (EvaHttpRequest  *request,
					                  gboolean         is_proxy_auth,
					                  EvaHttpAuthorization *auth);
EvaHttpAuthorization*
                eva_http_request_peek_authorization      (EvaHttpRequest  *request,
				                          gboolean    is_proxy_auth);
/* Requests */
EvaHttpRequest *eva_http_request_new_blank               (void);
EvaHttpRequest *eva_http_request_new                     (EvaHttpVerb  verb,
						          const char  *path);

typedef enum
{
  EVA_HTTP_REQUEST_FIRST_LINE_ERROR,
  EVA_HTTP_REQUEST_FIRST_LINE_SIMPLE,
  EVA_HTTP_REQUEST_FIRST_LINE_FULL
} EvaHttpRequestFirstLineStatus;

EvaHttpRequestFirstLineStatus
                eva_http_request_parse_first_line        (EvaHttpRequest *request,
				                          const char     *line,
                                                          GError        **error);

GHashTable     *eva_http_request_parse_cgi_query_string  (const char     *query_string);

/* XXX: duplicates eva_url_split_form_urlencoded! */
char **         eva_http_parse_cgi_query_string          (const char     *query_string,
                                                          GError        **error);

/* macros to get/set properties.  */
#define eva_http_request_set_verb(request, verb)	\
    g_object_set(EVA_HTTP_REQUEST(request), "verb", (EvaHttpVerb)(verb), NULL)
#define eva_http_request_get_verb(request) (EVA_HTTP_REQUEST(request)->verb)
#define eva_http_request_set_if_modified_since(request, t) \
    g_object_set(EVA_HTTP_REQUEST(request), "if-modified-since", (long)(t), NULL)
#define eva_http_request_get_if_modified_since(request) \
    (EVA_HTTP_REQUEST(request)->if_modified_since)
#define eva_http_request_set_from(request, from)			      \
  g_object_set (EVA_HTTP_REQUEST(request), "from", (const char *) (from), NULL)
#define eva_http_request_set_host(request, host)			      \
  g_object_set (EVA_HTTP_REQUEST(request), "host", (const char *) (host), NULL)
#define eva_http_request_peek_from(request)				      \
  (EVA_HTTP_REQUEST(request)->from)
#define eva_http_request_set_user_agent(request, user_agent)	              \
  g_object_set (EVA_HTTP_REQUEST(request), "user-agent", (const char *) (user_agent), NULL)
#define eva_http_request_peek_user_agent(request)		              \
  (EVA_HTTP_REQUEST(request)->user_agent)

G_END_DECLS

#endif
