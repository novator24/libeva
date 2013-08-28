/*
    EVA - a library to write servers
    Copyright (C) 1999-2000 Dave Benson

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA

    Contact:
        daveb@ffem.org <Dave Benson>
*/

/*
 * As per the HTTP 1.1 Specification, RFC 2616.
 *
 * TODO:  Compliance notes.
 */

#ifndef __EVA_HTTP_HEADER_H_
#define __EVA_HTTP_HEADER_H_

#include <glib-object.h>
#include "../evabuffer.h"

G_BEGIN_DECLS

typedef struct _EvaHttpHeaderClass EvaHttpHeaderClass;
typedef struct _EvaHttpHeader EvaHttpHeader;
typedef struct _EvaHttpAuthorization EvaHttpAuthorization;
typedef struct _EvaHttpAuthenticate EvaHttpAuthenticate;
typedef struct _EvaHttpCharSet EvaHttpCharSet;
typedef struct _EvaHttpResponseCacheDirective EvaHttpResponseCacheDirective;
typedef struct _EvaHttpRequestCacheDirective EvaHttpRequestCacheDirective;
typedef struct _EvaHttpCookie EvaHttpCookie;
typedef struct _EvaHttpLanguageSet EvaHttpLanguageSet;
typedef struct _EvaHttpMediaTypeSet EvaHttpMediaTypeSet;
typedef struct _EvaHttpContentEncodingSet EvaHttpContentEncodingSet;
typedef struct _EvaHttpTransferEncodingSet EvaHttpTransferEncodingSet;
typedef struct _EvaHttpRangeSet EvaHttpRangeSet;

/* enums */
GType eva_http_status_get_type (void) G_GNUC_CONST;
GType eva_http_verb_get_type (void) G_GNUC_CONST;
GType eva_http_content_encoding_get_type (void) G_GNUC_CONST;
GType eva_http_transfer_encoding_get_type (void) G_GNUC_CONST;
GType eva_http_range_get_type (void) G_GNUC_CONST;
GType eva_http_connection_get_type (void) G_GNUC_CONST;

/* boxed types */
GType eva_http_cookie_get_type (void) G_GNUC_CONST;
GType eva_http_authorization_get_type (void) G_GNUC_CONST;
GType eva_http_authenticate_get_type (void) G_GNUC_CONST;
GType eva_http_char_set_get_type (void) G_GNUC_CONST;
GType eva_http_cache_directive_get_type (void) G_GNUC_CONST;
GType eva_http_language_set_get_type (void) G_GNUC_CONST;
GType eva_http_media_type_set_get_type (void) G_GNUC_CONST;
GType eva_http_content_encoding_set_get_type (void) G_GNUC_CONST;
GType eva_http_transfer_encoding_set_get_type (void) G_GNUC_CONST;
GType eva_http_range_set_get_type (void) G_GNUC_CONST;

/* object types */
GType eva_http_header_get_type (void) G_GNUC_CONST;
GType eva_http_request_get_type (void) G_GNUC_CONST;
GType eva_http_response_get_type (void) G_GNUC_CONST;

/* type macros */
#define EVA_TYPE_HTTP_STATUS       (eva_http_status_get_type ())
#define EVA_TYPE_HTTP_VERB         (eva_http_verb_get_type ())
#define EVA_TYPE_HTTP_TRANSFER_ENCODING     (eva_http_transfer_encoding_get_type ())
#define EVA_TYPE_HTTP_CONTENT_ENCODING     (eva_http_content_encoding_get_type ())
#define EVA_TYPE_HTTP_RANGE        (eva_http_range_get_type ())
#define EVA_TYPE_HTTP_CONNECTION   (eva_http_connection_get_type ())
#define EVA_TYPE_HTTP_MEDIA_SET    (eva_http_media_set_get_type ())

/* type-casting macros */
#define EVA_TYPE_HTTP_HEADER               (eva_http_header_get_type ())
#define EVA_HTTP_HEADER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_HTTP_HEADER, EvaHttpHeader))
#define EVA_HTTP_HEADER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_HTTP_HEADER, EvaHttpHeaderClass))
#define EVA_HTTP_HEADER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_HTTP_HEADER, EvaHttpHeaderClass))
#define EVA_IS_HTTP_HEADER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_HTTP_HEADER))
#define EVA_IS_HTTP_HEADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_HTTP_HEADER))


typedef enum
{
  EVA_HTTP_STATUS_CONTINUE                      = 100,
  EVA_HTTP_STATUS_SWITCHING_PROTOCOLS           = 101,
  EVA_HTTP_STATUS_OK                            = 200,
  EVA_HTTP_STATUS_CREATED                       = 201,
  EVA_HTTP_STATUS_ACCEPTED                      = 202,
  EVA_HTTP_STATUS_NONAUTHORITATIVE_INFO         = 203,
  EVA_HTTP_STATUS_NO_CONTENT                    = 204,
  EVA_HTTP_STATUS_RESET_CONTENT                 = 205,
  EVA_HTTP_STATUS_PARTIAL_CONTENT               = 206,
  EVA_HTTP_STATUS_MULTIPLE_CHOICES              = 300,
  EVA_HTTP_STATUS_MOVED_PERMANENTLY             = 301,
  EVA_HTTP_STATUS_FOUND                         = 302,
  EVA_HTTP_STATUS_SEE_OTHER                     = 303,
  EVA_HTTP_STATUS_NOT_MODIFIED                  = 304,
  EVA_HTTP_STATUS_USE_PROXY                     = 305,
  EVA_HTTP_STATUS_TEMPORARY_REDIRECT            = 306,
  EVA_HTTP_STATUS_BAD_REQUEST                   = 400,
  EVA_HTTP_STATUS_UNAUTHORIZED                  = 401,
  EVA_HTTP_STATUS_PAYMENT_REQUIRED              = 402,
  EVA_HTTP_STATUS_FORBIDDEN                     = 403,
  EVA_HTTP_STATUS_NOT_FOUND                     = 404,
  EVA_HTTP_STATUS_METHOD_NOT_ALLOWED            = 405,
  EVA_HTTP_STATUS_NOT_ACCEPTABLE                = 406,
  EVA_HTTP_STATUS_PROXY_AUTH_REQUIRED           = 407,
  EVA_HTTP_STATUS_REQUEST_TIMEOUT               = 408,
  EVA_HTTP_STATUS_CONFLICT                      = 409,
  EVA_HTTP_STATUS_GONE                          = 410,
  EVA_HTTP_STATUS_LENGTH_REQUIRED               = 411,
  EVA_HTTP_STATUS_PRECONDITION_FAILED           = 412,
  EVA_HTTP_STATUS_ENTITY_TOO_LARGE              = 413,
  EVA_HTTP_STATUS_URI_TOO_LARGE                 = 414,
  EVA_HTTP_STATUS_UNSUPPORTED_MEDIA             = 415,
  EVA_HTTP_STATUS_BAD_RANGE                     = 416,
  EVA_HTTP_STATUS_EXPECTATION_FAILED            = 417,
  EVA_HTTP_STATUS_INTERNAL_SERVER_ERROR         = 500,
  EVA_HTTP_STATUS_NOT_IMPLEMENTED               = 501,
  EVA_HTTP_STATUS_BAD_GATEWAY                   = 502,
  EVA_HTTP_STATUS_SERVICE_UNAVAILABLE           = 503,
  EVA_HTTP_STATUS_GATEWAY_TIMEOUT               = 504,
  EVA_HTTP_STATUS_UNSUPPORTED_VERSION           = 505
} EvaHttpStatus;

/*
 * The Verb is the first text transmitted
 * (from the user-agent to the server).
 */
typedef enum
{
  EVA_HTTP_VERB_GET,
  EVA_HTTP_VERB_POST,
  EVA_HTTP_VERB_PUT,
  EVA_HTTP_VERB_HEAD,
  EVA_HTTP_VERB_OPTIONS,
  EVA_HTTP_VERB_DELETE,
  EVA_HTTP_VERB_TRACE,
  EVA_HTTP_VERB_CONNECT
} EvaHttpVerb;

/* A EvaHttpRange is a unit in which partial content ranges
 * may be specified and transferred.
 */
typedef enum
{
  EVA_HTTP_RANGE_BYTES
} EvaHttpRange;

typedef enum {
  EVA_HTTP_CONTENT_ENCODING_IDENTITY,
  EVA_HTTP_CONTENT_ENCODING_GZIP,
  EVA_HTTP_CONTENT_ENCODING_COMPRESS,
  EVA_HTTP_CONTENT_ENCODING_UNRECOGNIZED = 0x100
} EvaHttpContentEncoding;

/*
 * The Transfer-Encoding field of HTTP/1.1.
 *
 * In particular, HTTP/1.1 compliant clients and proxies
 * MUST support `chunked'.  The compressed Transfer-Encodings
 * are rarely (if ever) used.
 *
 * Note that:
 *   - we interpret this field, even for HTTP/1.0 clients.
 */
typedef enum {
  EVA_HTTP_TRANSFER_ENCODING_NONE    = 0,
  EVA_HTTP_TRANSFER_ENCODING_CHUNKED = 1,
  EVA_HTTP_TRANSFER_ENCODING_UNRECOGNIZED = 0x100
} EvaHttpTransferEncoding;

/*
 * The Connection: header enables or disables http-keepalive.
 *
 * For HTTP/1.0, NONE should be treated like CLOSE.
 * For HTTP/1.1, NONE should be treated like KEEPALIVE.
 *
 * Use eva_http_header_get_connection () to do this automatically -- it
 * always returns EVA_HTTP_CONNECTION_CLOSE or EVA_HTTP_CONNECTION_KEEPALIVE.
 *
 * See RFC 2616, Section 14.10.
 */
typedef enum
{
  EVA_HTTP_CONNECTION_NONE       = 0,
  EVA_HTTP_CONNECTION_CLOSE      = 1,
  EVA_HTTP_CONNECTION_KEEPALIVE  = 2,
} EvaHttpConnection;

/*
 * The Cache-Control response directive.
 * See RFC 2616, Section 14.9 (cache-response-directive)
 */
struct _EvaHttpResponseCacheDirective
{
  /*< public (read/write) >*/
  /* the http is `public' and `private'; is_ is added
   * for C++ users.
   */
  guint   is_public : 1;
  guint   is_private : 1;

  guint   no_cache : 1;
  guint   no_store : 1;
  guint   no_transform : 1;

  guint   must_revalidate : 1;
  guint   proxy_revalidate : 1;
  guint   max_age;
  guint   s_max_age;

  /*< public (read-only) >*/
  char   *private_name;
  char   *no_cache_name;

  /* TODO: what about cache-extensions? */
};

/*
 * The Cache-Control request directive.
 * See RFC 2616, Section 14.9 (cache-request-directive)
 */
struct _EvaHttpRequestCacheDirective
{
  guint no_cache : 1;
  guint no_store : 1;
  guint no_transform : 1;
  guint only_if_cached : 1;

  guint max_age;
  guint min_fresh;

 /* 
  * We need to be able to indicate that max_stale is set without the 
  * optional argument. So:
  *		      0 not set
  *		     -1 set no arg
  *		     >0 set with arg.	  
  */
  gint  max_stale;

  /* TODO: what about cache-extensions? */
};


EvaHttpResponseCacheDirective *eva_http_response_cache_directive_new (void);
void eva_http_response_cache_directive_set_private_name (
				     EvaHttpResponseCacheDirective *directive,
				     const char            *name,
				     gsize                  name_len);
void eva_http_response_cache_directive_set_no_cache_name (
				     EvaHttpResponseCacheDirective *directive,
				     const char            *name,
				     gsize                  name_len);
void eva_http_response_cache_directive_free(
				    EvaHttpResponseCacheDirective *directive);


EvaHttpRequestCacheDirective *eva_http_request_cache_directive_new (void);
void eva_http_request_cache_directive_free(
				     EvaHttpRequestCacheDirective *directive);


/*
 * The Accept: request-header.
 *
 * See RFC 2616, Section 14.1.
 *
 * TODO: support level= accept-extension.
 */
struct _EvaHttpMediaTypeSet
{
  /*< public: read-only >*/
  char                 *type;
  char                 *subtype;
  gfloat                quality;                /* -1 if not present */

  /*< private >*/
  EvaHttpMediaTypeSet  *next;
};
EvaHttpMediaTypeSet *eva_http_media_type_set_new (const char *type,
                                                  const char *subtype,
                                                  gfloat      quality);
void                 eva_http_media_type_set_free(EvaHttpMediaTypeSet *set);


/*
 * The Accept-Charset: request-header.
 *
 * See RFC 2616, Section 14.2.
 */
struct _EvaHttpCharSet
{
  /*< public: read-only >*/
  char                 *charset_name;
  gfloat                quality;                /* -1 if not present */

  /*< private >*/
  EvaHttpCharSet       *next;
};
EvaHttpCharSet       *eva_http_char_set_new (const char *charset_name,
                                             gfloat      quality);
void                  eva_http_char_set_free(EvaHttpCharSet *char_set);

/*
 * The Accept-Encoding: request-header.
 *
 * See RFC 2616, Section 14.3.
 */
struct _EvaHttpContentEncodingSet
{
  /*< public: read-only >*/
  EvaHttpContentEncoding       encoding;
  gfloat                       quality;       /* -1 if not present */

  /*< private >*/
  EvaHttpContentEncodingSet   *next;
};
EvaHttpContentEncodingSet *
eva_http_content_encoding_set_new (EvaHttpContentEncoding encoding,
				   gfloat                 quality);
void
eva_http_content_encoding_set_free(EvaHttpContentEncodingSet *encoding_set);

/*
 * for the TE: request-header.
 *
 * See RFC 2616, Section 14.39.
 */
struct _EvaHttpTransferEncodingSet
{
  /*< public: read-only >*/
  EvaHttpTransferEncoding      encoding;
  gfloat                       quality;       /* -1 if not present */

  /*< private >*/
  EvaHttpTransferEncodingSet   *next;
};
EvaHttpTransferEncodingSet *
eva_http_transfer_encoding_set_new (EvaHttpTransferEncoding     encoding,
				    gfloat                      quality);
void
eva_http_transfer_encoding_set_free(EvaHttpTransferEncodingSet *encoding_set);

struct _EvaHttpRangeSet
{
  /*< public: read-only >*/
  EvaHttpRange          range_type;

  /*< private >*/
  EvaHttpRangeSet   *next;
};
EvaHttpRangeSet *eva_http_range_set_new (EvaHttpRange range_type);
void             eva_http_range_set_free(EvaHttpRangeSet *set);


/*
 * The Accept-Language: request-header.
 *
 * See RFC 2616, Section 14.4.
 */
struct _EvaHttpLanguageSet
{
  /*< public: read-only >*/

  /* these give a language (with optional dialect specifier) */
  char                 *language;
  gfloat                quality;                /* -1 if not present */

  /*< private >*/
  EvaHttpLanguageSet   *next;
};
EvaHttpLanguageSet *eva_http_language_set_new (const char *language,
                                               gfloat      quality);
void                eva_http_language_set_free(EvaHttpLanguageSet *set);

typedef enum
{
  EVA_HTTP_AUTH_MODE_UNKNOWN,
  EVA_HTTP_AUTH_MODE_BASIC,
  EVA_HTTP_AUTH_MODE_DIGEST
} EvaHttpAuthMode;

/* HTTP Authentication.
   
   These structures give map to the WWW-Authenticate/Authorization headers,
   see RFC 2617.

   The outline is:
     If you get a 401 (Unauthorized) header, the server will
     accompany that with information about how to authenticate,
     in the WWW-Authenticate header.
     
     The user-agent should prompt the user for a username/password.

     Then the client tries again: but this time with an appropriate Authorization.
     If the server is satified, it will presumably give you the content.
 */
struct _EvaHttpAuthenticate
{
  EvaHttpAuthMode mode;
  char           *auth_scheme_name;
  char           *realm;
  union
  {
    struct {
      char                   *options;
    } unknown;
    /* no members:
    struct {
    } basic;
    */
    struct {
      char                   *domain;
      char                   *nonce;
      char                   *opaque;
      gboolean                is_stale;
      char                   *algorithm;
    } digest;
  } info;
  guint           ref_count;            /*< private >*/
};
EvaHttpAuthenticate *eva_http_authenticate_new_unknown (const char          *auth_scheme_name,
                                                        const char          *realm,
                                                        const char          *options);
EvaHttpAuthenticate *eva_http_authenticate_new_basic   (const char          *realm);
EvaHttpAuthenticate *eva_http_authenticate_new_digest  (const char          *realm,
                                                        const char          *domain,
                                                        const char          *nonce,
                                                        const char          *opaque,
                                                        const char          *algorithm);
EvaHttpAuthenticate  *eva_http_authenticate_ref        (EvaHttpAuthenticate *auth);
void                  eva_http_authenticate_unref      (EvaHttpAuthenticate *auth);

struct _EvaHttpAuthorization
{
  EvaHttpAuthMode mode;
  char           *auth_scheme_name;
  union
  {
    struct {
      char                   *response;
    } unknown;
    struct {
      char                   *user;
      char                   *password;
    } basic;
    struct {
      char                   *realm;
      char                   *domain;
      char                   *nonce;
      char                   *opaque;
      char                   *algorithm;
      char                   *user;
      char                   *password;
      char                   *response_digest;
      char                   *entity_digest;
    } digest;
  } info;
  guint           ref_count;            /*< private >*/
};
EvaHttpAuthorization *eva_http_authorization_new_unknown (const char *auth_scheme_name,
                                                          const char *response);
EvaHttpAuthorization *eva_http_authorization_new_basic   (const char *user,
                                                          const char *password);
EvaHttpAuthorization *eva_http_authorization_new_digest  (const char *realm,
                                                          const char *domain,
                                                          const char *nonce,
                                                          const char *opaque,
                                                          const char *algorithm,
                                                          const char *user,
                                                          const char *password,
                                                          const char *response_digest,
                                                          const char *entity_digest);
EvaHttpAuthorization *eva_http_authorization_new_respond (const EvaHttpAuthenticate *,
                                                          const char *user,
                                                          const char *password,
                                                          GError    **error);
EvaHttpAuthorization *eva_http_authorization_new_respond_post(const EvaHttpAuthenticate *,
                                                          const char *user,
                                                          const char *password,
                                                          guint       post_data_len,
                                                          gconstpointer post_data,
                                                          GError    **error);
const char           *eva_http_authorization_peek_response_digest (EvaHttpAuthorization *);
EvaHttpAuthorization *eva_http_authorization_copy        (const EvaHttpAuthorization *);
void                  eva_http_authorization_set_nonce   (EvaHttpAuthorization *,
                                                          const char *nonce);
EvaHttpAuthorization *eva_http_authorization_ref         (EvaHttpAuthorization *);
void                  eva_http_authorization_unref       (EvaHttpAuthorization *);

/* an update to an existing authentication,
   to verify that we're talking to the same host. */
struct _EvaHttpAuthenticateInfo
{
  char *next_nonce;
  char *response_digest;
  char *cnonce;
  guint has_nonce_count;
  guint32 nonce_count;
};

/* A single `Cookie' or `Set-Cookie' header.
 *
 * See RFC 2109, HTTP State Management Mechanism 
 */
struct _EvaHttpCookie
{
  /*< public: read-only >*/
  char                   *key;
  char                   *value;
  char                   *domain;
  char                   *path;
  char                   *expire_date;
  char                   *comment;
  int                     max_age;
  gboolean                secure;               /* default is FALSE */
  guint                   version;              /* default is 0, unspecified */
};
EvaHttpCookie  *eva_http_cookie_new              (const char     *key,
                                                  const char     *value,
                                                  const char     *path,
                                                  const char     *domain,
                                                  const char     *expire_date,
                                                  const char     *comment,
                                                  int             max_age);
EvaHttpCookie  *eva_http_cookie_copy             (const EvaHttpCookie *orig);
void            eva_http_cookie_free             (EvaHttpCookie *orig);


/*
 *                 EvaHttpHeader
 *
 * A structure embodying an http header
 * (as in a request or a response).
 */
struct _EvaHttpHeaderClass
{
  GObjectClass                  base_class;
};

struct _EvaHttpHeader
{
  GObject                       base_instance;

  /*< public >*/  /* read-write */
  guint16                       http_major_version;             /* always 1 */
  guint16                       http_minor_version;

  EvaHttpConnection             connection_type;

  EvaHttpTransferEncoding       transfer_encoding_type;
  EvaHttpContentEncoding        content_encoding_type;
  EvaHttpRangeSet              *accepted_range_units;  /* Accept-Ranges */

  /*< public >*/
  char                         *unrecognized_transfer_encoding;
  char                         *unrecognized_content_encoding;

  char                     *content_encoding;     /* Content-Encoding */

  unsigned                  has_content_type : 1;

  char                     *content_type;             /* Content-Type */
  char                     *content_subtype;
  char                     *content_charset;
  GSList                   *content_additional;

  /* NULL-terminated array of language tags from the Content-Language
   * header, or NULL if header not present. */
  char                    **content_languages;

  /* Bytes ranges. Both with be == -1 if there is no Range tag. */
  int                           range_start;
  int                           range_end;

  /* may be set to ((time_t) -1) to omit them. */
  glong                         date;

  /* From the Content-Length header. */
  gint64                        content_length;

  /*< public >*/

  /* Key/value searchable header lines. */
  GHashTable                   *header_lines;

  /* Error messages.  */
  GSList                       *errors;		      /* list of char* Error: directives */

  /* General headers.  */
  GSList                       *pragmas;

  /* and actual accumulated parse error (a bit of a hack) */
  GError                       *g_error;
};


/* Public methods to parse/write a header. */
typedef enum
{
  EVA_HTTP_PARSE_STRICT = (1 << 0),

  /* instead of giving up on unknown headers, just 
   * add them as misc-headers */
  EVA_HTTP_PARSE_SAVE_ERRORS = (1 << 1)
} EvaHttpParseFlags;

EvaHttpHeader  *eva_http_header_from_buffer      (EvaBuffer     *input,
						  gboolean       is_request,
                                                  EvaHttpParseFlags flags,
                                                  GError        **error);
void            eva_http_header_to_buffer        (EvaHttpHeader *header,
                                                  EvaBuffer     *output);


/* response specific functions */
/* unhandled: content-type and friends */
void             eva_http_header_set_content_encoding(EvaHttpHeader *header,
                                                     const char      *encoding);

/*content_type; content_subtype; content_charset; content_additional;*/

typedef struct _EvaHttpContentTypeInfo EvaHttpContentTypeInfo;
struct _EvaHttpContentTypeInfo
{
  const char *type_start;
  guint type_len;
  const char *subtype_start;
  guint subtype_len;
  const char *charset_start;
  guint charset_len;
  guint max_additional;         /* unimplemented */
  guint n_additional;           /* unimplemented */
  const char **additional_starts; /* unimplemented */
  guint *additional_lens;         /* unimplemented */
};
typedef enum
{
  EVA_HTTP_CONTENT_TYPE_PARSE_ADDL = (1<<0) /* unimplemented */
} EvaHttpContentTypeParseFlags;

gboolean eva_http_content_type_parse (const char *content_type_header,
                                      EvaHttpContentTypeParseFlags flags,
                                      EvaHttpContentTypeInfo *out,
                                      GError                **error);



/* --- miscellaneous key/value pairs --- */
void             eva_http_header_add_misc     (EvaHttpHeader *header,
                                               const char    *key,
                                               const char    *value);
void             eva_http_header_remove_misc  (EvaHttpHeader *header,
                                               const char    *key);
const char      *eva_http_header_lookup_misc  (EvaHttpHeader *header,
                                               const char    *key);

/* XXX: need to figure out the clean way to replace this one
 * (probably some generic eva_make_randomness (CHAR-SET, LENGTH) trick)
 */
/*char                *eva_http_header_gen_random_cookie();*/


typedef struct _EvaHttpHeaderLineParser EvaHttpHeaderLineParser;
typedef gboolean (*EvaHttpHeaderLineParserFunc) (EvaHttpHeader *header,
						 const char    *text,
						 gpointer       data);
struct _EvaHttpHeaderLineParser
{
  const char *name;
  EvaHttpHeaderLineParserFunc func;
  gpointer data;
};
  
/* The returned table is a map from g_str_hash(lowercase(header)) to
   EvaHttpHeaderLineParser. */
GHashTable        *eva_http_header_get_parser_table(gboolean       is_request);

/* Standard header constructions... */


/* --- outputting an http header --- */
typedef void (*EvaHttpHeaderPrintFunc) (const char       *text,
					gpointer          data);
void              eva_http_header_print (EvaHttpHeader          *http_header,
		                         EvaHttpHeaderPrintFunc  print_func,
		                         gpointer                print_data);


EvaHttpConnection    eva_http_header_get_connection (EvaHttpHeader *header);
void                 eva_http_header_set_version    (EvaHttpHeader *header,
						     gint           major,
						     gint           minor);
void                 eva_http_header_add_pragma     (EvaHttpHeader *header,
                                                     const char    *pragma);
void             eva_http_header_add_accepted_range (EvaHttpHeader *header,
                                                     EvaHttpRange   range);


#define eva_http_header_set_connection_type(header, connection_type)	      \
  g_object_set (EVA_HTTP_HEADER(header), "connection", (EvaHttpConnection) (connection_type), NULL)
#define eva_http_header_get_connection_type(header)			      \
  (EVA_HTTP_HEADER(header)->connection_type)
#define eva_http_header_set_transfer_encoding(header, enc)		      \
  g_object_set (EVA_HTTP_HEADER(header), "transfer-encoding", (EvaHttpTransferEncoding) (enc), NULL)
#define eva_http_header_get_transfer_encoding(header)			      \
  (EVA_HTTP_HEADER(header)->transfer_encoding_type)
#define eva_http_header_set_content_encoding(header, enc)		      \
  g_object_set (EVA_HTTP_HEADER(header), "content-encoding", (EvaHttpContentEncoding) (enc), NULL)
#define eva_http_header_get_content_encoding(header)			      \
  (EVA_HTTP_HEADER(header)->content_encoding_type)
#define eva_http_header_set_content_length(header, len)		              \
  g_object_set (EVA_HTTP_HEADER(header), "content-length", (gint64) (len), NULL)
#define eva_http_header_get_content_length(header)			      \
  (EVA_HTTP_HEADER(header)->content_length)
#define eva_http_header_set_range(header, start, end)		              \
  g_object_set (EVA_HTTP_HEADER(header), "range-start", (gint) (start), "range-end", (gint) (end), NULL)
#define eva_http_header_get_range_start(header)			              \
  (EVA_HTTP_HEADER(header)->range_start)
#define eva_http_header_get_range_end(header)			              \
  (EVA_HTTP_HEADER(header)->range_end)
#define eva_http_header_set_date(header, date)			      	      \
  g_object_set (EVA_HTTP_HEADER(header), "date", (long) (date), NULL)
#define eva_http_header_get_date(header)				      \
  (EVA_HTTP_HEADER(header)->date)

/*< private >*/
void eva_http_header_set_string (gpointer         http_header,
                                 char           **p_str,
                                 const char      *str);
void eva_http_header_set_string_len (gpointer         http_header,
                                     char           **p_str,
                                     const char      *str,
                                     guint            len);

void eva_http_header_set_string_val (gpointer         http_header,
                                     char           **p_str,
                                     const GValue    *value);

char * eva_http_header_cut_string (gpointer    http_header,
                                   const char *start,
                                   const char *end);

void eva_http_header_free_string (gpointer http_header,
			          char    *str);
void eva_http_header_set_connection_string (EvaHttpHeader *header,
                                            const char    *str);
void eva_http_header_set_content_encoding_string  (EvaHttpHeader *header,
                                                   const char    *str);
void eva_http_header_set_transfer_encoding_string (EvaHttpHeader *header,
                                                   const char    *str);

#define eva_http_header_set_content_type(header, content_type)	      \
  g_object_set (EVA_HTTP_HEADER(header), "content-type", (const char *)(content_type), NULL)
#define eva_http_header_get_content_type(header)			      \
  (EVA_HTTP_HEADER(header)->content_type)
#define eva_http_header_set_content_subtype(header, content_type)	      \
  g_object_set (EVA_HTTP_HEADER(header), "content-subtype", (const char *)(content_type), NULL)
#define eva_http_header_get_content_subtype(header)			      \
  (EVA_HTTP_HEADER(header)->content_subtype)
#define eva_http_header_set_content_charset(header, content_type)	      \
  g_object_set (EVA_HTTP_HEADER(header), "content-charset", (const char *)(content_type), NULL)
#define eva_http_header_get_content_charset(header)			      \
  (EVA_HTTP_HEADER(header)->content_charset)

G_END_DECLS

#endif
