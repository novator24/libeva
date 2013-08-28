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


#ifndef __EVA_URL_H_
#define __EVA_URL_H_

#include "../evastream.h"

G_BEGIN_DECLS

typedef struct _EvaUrl EvaUrl;
typedef struct _EvaUrlClass EvaUrlClass;

/* --- type macros --- */
GType eva_url_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_URL			(eva_url_get_type ())
#define EVA_URL(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_URL, EvaUrl))
#define EVA_URL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_URL, EvaUrlClass))
#define EVA_URL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_URL, EvaUrlClass))
#define EVA_IS_URL(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_URL))
#define EVA_IS_URL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_URL))

/* --- enums --- */
typedef enum 
{
  EVA_URL_SCHEME_FILE,
  EVA_URL_SCHEME_HTTP,
  EVA_URL_SCHEME_HTTPS,
  EVA_URL_SCHEME_FTP,
  EVA_URL_SCHEME_OTHER = 0x10000,
} EvaUrlScheme;

/* --- structures --- */
struct _EvaUrlClass
{
  GObjectClass                  parent_class;
};

struct _EvaUrl
{
  GObject                       parent_instance;

  EvaUrlScheme			scheme;
  char                         *scheme_name;

  /* The place where the resource can be obtained.
   * (The RFC refers to this as an `authority', which is more general, sect 3.2)
   */
  char                         *host;

  /* XXX: refer to RFC again for if this is general... */
  char                         *password;

  /* For server-based urls, the port where the resource may be obtained,
   * or 0, indicating that none was specified. (Section 3.2.2)
   */
  int				port;

  /* The username or NULL if none was given. */
  char			       *user_name;

  /* The scheme-specific data (we separate the section and query pieces though)
   * This string usually begins with a slash (/).
   * RFC 2396, Section 3.3.
   */
  char                         *path;

  /* The query portion of the URL. */
  char                         *query;

  /* The portion after a `#' character. (Section 4.1) */
  char			       *fragment;
};

/* --- character sets --- */
/* Characters having specific meaning within a url; 
 * these should be escaped to be passed on to the underlying scheme.
 * From RFC 2396, section 2.2.
 */
#define EVA_URL_RESERVED_CHARSET			\
	";/?:@&=+$,"

/* Characters which do not normally need escaping within a URL.
 * From RFC 2396, section 2.3.
 */
#define EVA_URL_UNRESERVED_CHARSET			\
	"abcdefghijklmnopqrstuvwxyz"			\
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"			\
	"0123456789"					\
	"-_.!~*'()"

/* --- public methods --- */
EvaUrl         *eva_url_new                 (const char      *spec,
					     GError         **error);
EvaUrl         *eva_url_new_in_context      (const char      *spec,
                                             EvaUrlScheme     context,
					     GError         **error);
EvaUrl         *eva_url_new_from_parts      (EvaUrlScheme     scheme,
					     const char      *host,
					     int              port,
					     const char      *user_name,
					     const char      *password,
					     const char      *path,
					     const char      *query,
					     const char      *fragment);
EvaUrl         *eva_url_new_relative        (EvaUrl          *base_url,
					     const char      *location,
					     GError         **error);

char           *eva_url_get_relative_path   (EvaUrl          *url);

guint           eva_url_get_port            (const EvaUrl    *url);

/* Url-encoding helper functions. */
char           *eva_url_decode              (const char      *encoded);
char           *eva_url_encode              (const char      *decoded);


char           *eva_url_to_string           (const EvaUrl    *url);

guint           eva_url_hash                (const EvaUrl    *url);
gboolean        eva_url_equal               (const EvaUrl    *a,
                                             const EvaUrl    *b);

/* These do what is typically thought of
 * as "url encoding" in http-land... namely SPACE maps to '+'
 * and funny characters are encoded
 * as %xx where 'x' denotes a single hex-digit.
 * 
 * This is the encoding applied to field names and values in HTML
 * forms when using the default "application/x-www-form-urlencoded"
 * encoding. See RFC 1866, section 8.2.1.
 */
char           *eva_url_decode_http         (const char      *encoded);
char           *eva_url_encode_http         (const char      *decoded);
char           *eva_url_encode_http_binary  (const guint8    *decoded,
                                             guint            length);

/* Split an "application/x-www-form-urlencoded" format query string into
 * a null-terminated array of strings: key, value, ...
 * Caller must free result with g_strfreev.
 *
 * XXX: this is a duplicate of eva_http_parse_cgi_query_string()
 */
char **         eva_url_split_form_urlencoded (const char *encoded_query);


gboolean eva_url_is_valid_hostname          (const char *str,
                                             char *bad_char_out);
#define eva_url_is_valid_path(str, bad_char_out)     \
	eva_url_is_valid_generic_component(str, bad_char_out)
#define eva_url_is_valid_query(str, bad_char_out)    \
	eva_url_is_valid_generic_component(str, bad_char_out)
#define eva_url_is_valid_fragment(str, bad_char_out) \
	eva_url_is_valid_generic_component(str, bad_char_out)

/* Downloading URLs */
typedef void (*EvaUrlSuccess) (EvaStream        *stream,
			       gpointer          user_data);
typedef void (*EvaUrlFailure) (GError           *error,
			       gpointer          user_data);
void           eva_url_download             (EvaUrl          *url,
					     EvaUrlSuccess    success_func,
					     EvaUrlFailure    failure_func,
					     gpointer         user_data);

/* --- protected methods --- */
/* Allocating new url schemes. */
typedef EvaUrl *(*EvaUrlParser) (EvaUrlScheme     scheme,
				 const char      *url,
				 gpointer         data);
typedef char   *(*EvaUrlToString) (EvaUrl        *url,
				   gpointer       data);
EvaUrlScheme    eva_url_scheme_get_unique   (const char      *url_scheme,
					     guint            default_port,
					     EvaUrlParser     parse_func,
					     EvaUrlToString   print_func,
					     gpointer         data);

/* registering new download methods */
typedef struct _EvaUrlDownload EvaUrlDownload;
typedef void  (*EvaUrlDownloadMethod)       (EvaUrlDownload  *download,
					     gpointer         download_data);
void            eva_url_scheme_add_dl_method(EvaUrlScheme     scheme,
					     EvaUrlDownloadMethod download_method,
					     gpointer         download_data);

/* protected: (called (eventually) by DownloadMethod) */
void            eva_url_download_success    (EvaUrlDownload  *download,
			                     EvaStream       *stream);
void            eva_url_download_fail       (EvaUrlDownload  *download,
			                     GError          *error);
EvaUrl         *eva_url_download_peek_url   (EvaUrlDownload  *download);
void            eva_url_download_redirect   (EvaUrlDownload  *download,
					     EvaUrl          *new_url);

/*< private >*/
gboolean eva_url_is_valid_generic_component (const char *str,
                                             char *bad_char_out);
G_END_DECLS

#endif
