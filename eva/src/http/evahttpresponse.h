#ifndef __EVA_HTTP_RESPONSE_H_
#define __EVA_HTTP_RESPONSE_H_

#ifndef __EVA_HTTP_HEADER_H_
#include "evahttpheader.h"
#endif

G_BEGIN_DECLS

typedef struct _GskHttpResponseClass GskHttpResponseClass;
typedef struct _GskHttpResponse GskHttpResponse;

#define EVA_TYPE_HTTP_RESPONSE             (eva_http_response_get_type ())
#define EVA_HTTP_RESPONSE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_HTTP_RESPONSE, GskHttpResponse))
#define EVA_HTTP_RESPONSE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_HTTP_RESPONSE, GskHttpResponseClass))
#define EVA_HTTP_RESPONSE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_HTTP_RESPONSE, GskHttpResponseClass))
#define EVA_IS_HTTP_RESPONSE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_HTTP_RESPONSE))
#define EVA_IS_HTTP_RESPONSE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_HTTP_RESPONSE))

struct _GskHttpResponseClass
{
  GskHttpHeaderClass base_class;
};
struct _GskHttpResponse
{
  GskHttpHeader base_instance;
  
  GskHttpStatus             status_code;
  int                       age;                  /* Age */

  /* initially allowed_verbs == 0;
   * since it is an error not to allow any verbs;
   * otherwise it is a bitwise-OR: (1 << EVA_HTTP_VERB_*)
   */
  guint                     allowed_verbs;

  GskHttpResponseCacheDirective *cache_control;        /* Cache-Control */

  unsigned                  has_md5sum : 1;
  unsigned char             md5sum[16];           /* Content-MD5 (14.15) */

  /* List of Set-Cookie: headers. */
  GSList                   *set_cookies;

  /* The `Location' to redirect to. */
  char                     *location;

  /* may be set to ((time_t) -1) to omit them. */
  glong                     expires;
  char                     *expires_str;

  /* The ``Entity-Tag'', cf RFC 2616, Sections 14.24, 14.26, 14.44. */
  char                     *etag;

  GskHttpAuthenticate      *proxy_authenticate;

  /* This is the WWW-Authenticate: header line. */
  GskHttpAuthenticate      *authenticate;

  /* If `retry_after_relative', the retry_after is the number 
   * of seconds to wait before retrying; otherwise,
   * it is a unix-time indicting when to retry.
   *
   * (Corresponding to the `Retry-After' header, cf RFC 2616, 14.37)
   */
  guint                     has_retry_after : 1;
  gboolean                  retry_after_relative;
  glong                     retry_after;

  /* The Last-Modified header.  If != -1, this is the unix-time
   * the message-body-contents were last modified. (RFC 2616, section 14.29)
   */
  glong                     last_modified;

  char                     *server;        /* The Server: header */
};

/* Responses. */
GskHttpResponse *eva_http_response_new_blank    (void);

/* Redirects should be accompanied by an HTML body saying the URL. */
GskHttpResponse *eva_http_response_new_redirect (const char    *location);

GskHttpResponse *eva_http_response_from_request (GskHttpRequest *request,
						 GskHttpStatus   status_code,
						 gint64          length);

gboolean   eva_http_response_process_first_line (GskHttpResponse *response,
				                 const char      *line);

void       eva_http_response_set_retry_after   (GskHttpResponse *response,
                                                gboolean         is_relative,
                                                glong            time);
void       eva_http_response_set_no_retry_after(GskHttpResponse *response);

void       eva_http_response_set_authenticate (GskHttpResponse *response,
					       gboolean         is_proxy_auth,
					       GskHttpAuthenticate *auth);
GskHttpAuthenticate*
           eva_http_response_peek_authenticate(GskHttpResponse *response,
				               gboolean         is_proxy_auth);

/* --- setting / getting --- */
gboolean   eva_http_response_has_content_body   (GskHttpResponse *response,
                                                 GskHttpRequest  *request);
void       eva_http_response_set_cache_control(
					    GskHttpResponse *response,
				            GskHttpResponseCacheDirective *directive);
#define    eva_http_response_set_status_code(response, status)	\
  G_STMT_START{ EVA_HTTP_RESPONSE(response)->status_code = (status); G_STMT_END
#define    eva_http_response_get_status_code(response) \
               (EVA_HTTP_RESPONSE(response)->status_code)
void       eva_http_response_set_allowed_verbs  (GskHttpResponse *response,
                                                 guint            allowed);
#define eva_http_response_get_allowed_verbs(header)		              \
  (EVA_HTTP_RESPONSE(header)->allowed_verbs)
/* md5sum may be NULL to unset it */
void             eva_http_response_set_md5      (GskHttpResponse *response,
                                                 const guint8    *md5sum);
const guint8    *eva_http_response_peek_md5     (GskHttpResponse *response);
#define eva_http_response_set_location(response, location)		      \
  g_object_set (EVA_HTTP_RESPONSE(response), "location", (const char *) (location), NULL)
#define eva_http_response_peek_location(response)		              \
  (EVA_HTTP_RESPONSE(response)->location)
#define eva_http_response_set_etag(response, etag)			      \
  g_object_set (EVA_HTTP_RESPONSE(response), "etag", (const char *)(etag), NULL)
#define eva_http_response_peek_etag(response)				      \
  (EVA_HTTP_RESPONSE(response)->etag)
#define eva_http_response_set_server(response, server)			      \
  g_object_set (EVA_HTTP_RESPONSE(response), "server", (const char *) (server), NULL)
#define eva_http_response_peek_server(response)				      \
  (EVA_HTTP_RESPONSE(response)->server)
#define eva_http_response_set_expires(response, expires)	              \
  g_object_set (EVA_HTTP_RESPONSE(response), "expires", (long) (expires), NULL)
#define eva_http_response_get_expires(response)				      \
  (EVA_HTTP_RESPONSE(response)->expires)
#define eva_http_response_peek_cache_control(response)	                      \
	(EVA_HTTP_RESPONSE(response)->cache_control)
#define eva_http_response_set_age(response, age)	              	      \
  g_object_set (EVA_HTTP_RESPONSE(response), "age", (long) (age), NULL)
#define eva_http_response_get_age(response)		                      \
	(EVA_HTTP_RESPONSE(response)->age)
#define eva_http_response_set_last_modified(response, last_modified)	      \
  g_object_set (EVA_HTTP_RESPONSE(response), "last-modified", (long) (last_modified), NULL)
#define eva_http_response_get_last_modified(response)		              \
  (EVA_HTTP_RESPONSE(response)->last_modified)

#define eva_http_response_set_content_type eva_http_header_set_content_type
#define eva_http_response_get_content_type eva_http_header_get_content_type
#define eva_http_response_set_content_subtype eva_http_header_set_content_subtype
#define eva_http_response_get_content_subtype eva_http_header_get_content_subtype

G_END_DECLS

#endif
