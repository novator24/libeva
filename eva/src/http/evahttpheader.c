#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "evahttpheader.h"
#include "../evaerror.h"
#include "../evamacros.h"

static GObjectClass *parent_class = NULL;
static GEnumClass *eva_http_connection_class = NULL;

#define ACTUAL_LENGTH(str)	((str) ? (strlen (str) + 1) : 0)

enum
{
  PROP_HEADER_0,
  PROP_HEADER_MAJOR_VERSION,
  PROP_HEADER_MINOR_VERSION,
  PROP_HEADER_CONNECTION,
  PROP_HEADER_CONNECTION_STRING,
  PROP_HEADER_CONTENT_ENCODING,
  PROP_HEADER_CONTENT_TYPE,
  PROP_HEADER_CONTENT_SUBTYPE,
  PROP_HEADER_CONTENT_CHARSET,
  PROP_HEADER_TRANSFER_ENCODING,
  PROP_HEADER_CONTENT_ENCODING_STRING,
  PROP_HEADER_TRANSFER_ENCODING_STRING,
  PROP_HEADER_CONTENT_LENGTH,
  PROP_HEADER_RANGE_START,
  PROP_HEADER_RANGE_END,
  PROP_HEADER_DATE
};

/* --- string setting --- */

/**
 * eva_http_header_set_string:
 * @http_header: HTTP header which owns the string.
 * @p_str: pointer to the string's location.
 * @str: string to copy and assign to *@p_str.
 * (May be NULL)
 *
 * Private function to set a string in a HTTP header.
 * (Currently this uses strdup, but we may eventually switch
 * allocation strategies).
 */
void
eva_http_header_set_string (gpointer         http_header,
                            char           **p_str,
                            const char      *str)
{
  char *cpy;
  g_return_if_fail (EVA_IS_HTTP_HEADER (http_header));
  cpy = g_strdup (str);
  if (*p_str)
    g_free (*p_str);
  *p_str = cpy;
}

void
eva_http_header_set_string_len (gpointer         http_header,
                                char           **p_str,
                                const char      *str,
                                guint            len)
{
  char *cpy;
  g_return_if_fail (EVA_IS_HTTP_HEADER (http_header));
  cpy = g_strndup (str, len);
  if (*p_str)
    g_free (*p_str);
  *p_str = cpy;
}

/**
 * eva_http_header_set_string_val:
 * @http_header: HTTP header which owns the string.
 * @p_str: pointer to the string's location.
 * @value: a value which holds a string.
 *
 * Private function to set a string in a HTTP header from a value.
 * This is used from the set_property methods in the various HTTP header
 * class implementations.
 */
void
eva_http_header_set_string_val (gpointer         http_header,
                                char           **p_str,
                                const GValue    *value)
{
  eva_http_header_set_string (http_header, p_str,
                              g_value_get_string (value));
}

/**
 * eva_http_header_cut_string:
 * @http_header: HTTP header which owns the string.
 * @start: the start of the string
 * @end: immediately past the input string.
 *
 * private.
 * Copy a substring assuming that http_header is responsible
 * for it.
 *
 * returns: the NUL-terminated copy of the string between @start
 * and @end.
 */
char *
eva_http_header_cut_string (gpointer    http_header,
                            const char *start,
                            const char *end)
{
  char *rv = g_new (char, end - start + 1);
  memcpy (rv, start, end - start);
  rv[end - start] = 0;
  return rv;
}

/**
 * eva_http_header_free_string:
 * @http_header: HTTP header which owns the string.
 * @str: the string to free.
 *
 * Deallocate a string allocated with 
 * eva_http_header_set_string(),
 * eva_http_header_set_string_val(),
 * eva_http_header_cut_string().
 */
void
eva_http_header_free_string (gpointer http_header,
			     char    *str)
{
  g_return_if_fail (EVA_IS_HTTP_HEADER (http_header));
  g_free (str);
}


/* --- EvaHttpHeader implementation --- */
/**
 * eva_http_header_set_connection_string:
 * @header: the HTTP header to affect.
 * @str: connection type, as a string.  (The string is exactly
 * the same as that which is found literally in the header.)
 *
 * Change the Connection type reflected in this header.
 *
 * [From RFC 2616, Section 14.10]
 * The Connection general-header field allows the sender to specify
 * options that are desired for that particular connection and MUST NOT
 * be communicated by proxies over further connections.
 */
void
eva_http_header_set_connection_string (EvaHttpHeader *header,
                                       const char    *str)
{
  char *tmp = g_ascii_strdown (str, -1);
  GEnumValue *enum_value = g_enum_get_value_by_nick (eva_http_connection_class, tmp);
  g_free (tmp);
  if (enum_value != NULL)
    header->connection_type = enum_value->value;
  else
    header->connection_type = EVA_HTTP_CONNECTION_CLOSE;
}

/**
 * eva_http_header_set_content_encoding_string:
 * @header: the HTTP header to affect.
 * @str: content-encoding type, as a string.  (The string is exactly
 * the same as that which is found literally in the header.)
 *
 * Change the Content-Encoding type reflected in this header.
 *
 * The actual encoding is done transparently by #EvaHttpClient and #EvaHttpServer usually.
 * Encoding is only used for POST and PUT requests
 * and any response that has a body (most do).
 * The default is the identity encoding.
 */
void
eva_http_header_set_content_encoding_string (EvaHttpHeader *header,
                                             const char    *str)
{
  if (g_ascii_strcasecmp (str, "identity") == 0)
    header->content_encoding_type = EVA_HTTP_CONTENT_ENCODING_IDENTITY;
  else if (g_ascii_strcasecmp (str, "gzip") == 0)
    header->content_encoding_type = EVA_HTTP_CONTENT_ENCODING_GZIP;
  else if (g_ascii_strcasecmp (str, "compress") == 0)
    header->content_encoding_type = EVA_HTTP_CONTENT_ENCODING_COMPRESS;
  else
    {
      header->content_encoding_type = EVA_HTTP_CONTENT_ENCODING_UNRECOGNIZED;
      header->unrecognized_content_encoding = g_ascii_strdown (str, -1);
    }
  if (header->content_encoding_type != EVA_HTTP_CONTENT_ENCODING_UNRECOGNIZED
   && header->unrecognized_content_encoding != NULL)
    {
      g_free (header->unrecognized_content_encoding);
      header->unrecognized_content_encoding = NULL;
    }
}

/**
 * eva_http_header_set_transfer_encoding_string:
 * @header: the HTTP header to affect.
 * @str: transfer-encoding type, as a string.  (The string is exactly
 * the same as that which is found literally in the header.)
 *
 * Set the Transfer-Encoding type, as a string.
 *
 * [From RFC 2616, Section 14.41]
 * The Transfer-Encoding general-header field indicates what (if any)
 * type of transformation has been applied to the message body in order
 * to safely transfer it between the sender and the recipient. This
 * differs from the content-coding in that the transfer-coding is a
 * property of the message, not of the entity.
 */
void
eva_http_header_set_transfer_encoding_string (EvaHttpHeader *header,
                                              const char    *str)
{
  if (g_ascii_strcasecmp (str, "none") == 0)
    header->transfer_encoding_type = EVA_HTTP_TRANSFER_ENCODING_NONE;
  else if (g_ascii_strcasecmp (str, "chunked") == 0)
    header->transfer_encoding_type = EVA_HTTP_TRANSFER_ENCODING_CHUNKED;
  else
    {
      header->transfer_encoding_type = EVA_HTTP_TRANSFER_ENCODING_UNRECOGNIZED;
      header->unrecognized_transfer_encoding = g_ascii_strdown (str, -1);
    }
  if (header->transfer_encoding_type != EVA_HTTP_TRANSFER_ENCODING_UNRECOGNIZED
   && header->unrecognized_transfer_encoding != NULL)
    {
      g_free (header->unrecognized_transfer_encoding);
      header->unrecognized_transfer_encoding = NULL;
    }
}

static void
eva_http_header_set_property   (GObject        *object,
                                guint           property_id,
                                const GValue   *value,
                                GParamSpec     *pspec)
{
  EvaHttpHeader *header = EVA_HTTP_HEADER (object);
  switch (property_id)
    {
    case PROP_HEADER_MAJOR_VERSION:
      header->http_major_version = g_value_get_uint (value);
      break;
    case PROP_HEADER_MINOR_VERSION:
      header->http_minor_version = g_value_get_uint (value);
      break;
    case PROP_HEADER_CONNECTION:
      header->connection_type = g_value_get_enum (value);
      break;
    case PROP_HEADER_CONTENT_ENCODING:
      header->content_encoding_type = g_value_get_enum (value);
      break;
#if 0
    case PROP_HEADER_CONTENT_ENCODING:
      eva_http_header_set_string_val (response, &response->content_encoding, value);
      break;
#endif
    case PROP_HEADER_CONTENT_TYPE:
      eva_http_header_set_string_val (header, &header->content_type, value);
      header->has_content_type = 1;		/* why is this needed? */
      break;
    case PROP_HEADER_CONTENT_SUBTYPE:
      eva_http_header_set_string_val (header, &header->content_subtype, value);
      break;
    case PROP_HEADER_CONTENT_CHARSET:
      eva_http_header_set_string_val (header, &header->content_charset, value);
      break;
    case PROP_HEADER_TRANSFER_ENCODING:
      header->transfer_encoding_type = g_value_get_enum (value);
      break;
    case PROP_HEADER_CONNECTION_STRING:
      eva_http_header_set_connection_string (header, g_value_get_string (value));
      break;
    case PROP_HEADER_TRANSFER_ENCODING_STRING:
      eva_http_header_set_transfer_encoding_string (header, g_value_get_string (value));
      break;
    case PROP_HEADER_CONTENT_ENCODING_STRING:
      eva_http_header_set_content_encoding_string (header, g_value_get_string (value));
      break;
    case PROP_HEADER_CONTENT_LENGTH:
      header->content_length = g_value_get_int (value);
      break;
    case PROP_HEADER_RANGE_START:
      header->range_start = g_value_get_int (value);
      break;
    case PROP_HEADER_RANGE_END:
      header->range_end = g_value_get_int (value);
      break;
    case PROP_HEADER_DATE:
      header->date = g_value_get_long (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
eva_http_header_get_property   (GObject        *object,
                                guint           property_id,
                                GValue         *value,
                                GParamSpec     *pspec)
{
  EvaHttpHeader *header = EVA_HTTP_HEADER (object);
  switch (property_id)
    {
    case PROP_HEADER_MAJOR_VERSION:
      g_value_set_uint (value, header->http_major_version);
      break;
    case PROP_HEADER_MINOR_VERSION:
      g_value_set_uint (value, header->http_minor_version);
      break;
    case PROP_HEADER_CONNECTION:
      g_value_set_enum (value, header->connection_type);
      break;
    case PROP_HEADER_CONTENT_ENCODING:
      g_value_set_enum (value, header->content_encoding_type);
      break;
    case PROP_HEADER_CONTENT_TYPE:
      g_value_set_string (value, header->content_type);
      break;
    case PROP_HEADER_CONTENT_SUBTYPE:
      g_value_set_string (value, header->content_subtype);
      break;
    case PROP_HEADER_CONTENT_CHARSET:
      g_value_set_string (value, header->content_charset);
      break;
    case PROP_HEADER_TRANSFER_ENCODING:
      g_value_set_enum (value, header->transfer_encoding_type);
      break;
    case PROP_HEADER_CONNECTION_STRING:
      {
        GEnumValue *enum_value = g_enum_get_value (eva_http_connection_class, header->connection_type);
        const char *str = enum_value ? enum_value->value_nick : "unknown";
        g_value_set_string (value, str);
      }
      break;
    case PROP_HEADER_CONTENT_ENCODING_STRING:
      {
        switch (header->transfer_encoding_type)
          {
	  case EVA_HTTP_CONTENT_ENCODING_IDENTITY:
            g_value_set_string (value, "none");
            break;
          case EVA_HTTP_CONTENT_ENCODING_GZIP:
            g_value_set_string (value, "gzip");
            break;
          case EVA_HTTP_CONTENT_ENCODING_COMPRESS:
            g_value_set_string (value, "compress");
            break;
          case EVA_HTTP_CONTENT_ENCODING_UNRECOGNIZED:
            g_value_set_string (value, header->unrecognized_content_encoding);
            break;
          default:
            g_value_set_string (value, "unknown");
            break;
          }
      }
      break;
    case PROP_HEADER_TRANSFER_ENCODING_STRING:
      {
        switch (header->transfer_encoding_type)
          {
          case EVA_HTTP_TRANSFER_ENCODING_NONE:
            g_value_set_string (value, "none");
            break;
          case EVA_HTTP_TRANSFER_ENCODING_CHUNKED:
            g_value_set_string (value, "chunked");
            break;
          case EVA_HTTP_TRANSFER_ENCODING_UNRECOGNIZED:
            g_value_set_string (value, header->unrecognized_transfer_encoding);
            break;
          default:
            g_value_set_string (value, "unknown");
            break;
          }
      }
      break;
    case PROP_HEADER_CONTENT_LENGTH:
      g_value_set_int (value, header->content_length);
      break;
    case PROP_HEADER_RANGE_START:
      g_value_set_int (value, header->range_start);
      break;
    case PROP_HEADER_RANGE_END:
      g_value_set_int (value, header->range_end);
      break;
    case PROP_HEADER_DATE:
      g_value_set_long (value, header->date);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * eva_http_header_get_connection:
 * @header: the HTTP header to query.
 *
 * Returns the connection semantics.
 * Note that this will never return #EVA_HTTP_CONNECTION_NONE;
 * it will figure out if the default is keep-alive or close.
 *
 * Use eva_http_header_get_connection_type() to get the 
 * actual connection-type that will be transmitted over the
 * wire (with NONE meaning that there is no Connection: header).
 *
 * Note that the client can make Connection: requests,
 * but the server is allowed to ignore keep-alive directives.
 * So calling this on the request gets the client's suggestions,
 * and calling this on the response gets the server's intentions.
 *
 * returns: the connection semantics.
 */
EvaHttpConnection
eva_http_header_get_connection (EvaHttpHeader *header)
{
  if (header->connection_type == EVA_HTTP_CONNECTION_NONE)
    {
      if (header->http_major_version == 0
       || (header->http_major_version == 1
        && header->http_minor_version == 0))
	return EVA_HTTP_CONNECTION_CLOSE;
      if (header->content_length >= 0
       || header->transfer_encoding_type == EVA_HTTP_TRANSFER_ENCODING_CHUNKED)
	return EVA_HTTP_CONNECTION_KEEPALIVE;
      /* default */
      return EVA_HTTP_CONNECTION_CLOSE;
    }

  /* TODO: sanity checks */

  return header->connection_type;
}

/**
 * eva_http_header_set_version:
 * @header: the HTTP header to affect.
 * @major: the major HTTP version number.
 * @minor: the minor HTTP version number.
 *
 * Set the major and minor versions;
 * for example, to use HTTP 1.0, one would set @major to 1,
 * and @minor to 0.
 *
 * In addition to setting the version numbers,
 * this function suppresses features not available in
 * the respective http implementations.
 *
 * Only HTTP 1.0 and 1.1 are supported.
 */
void
eva_http_header_set_version    (EvaHttpHeader *header,
				gint           major,
				gint           minor)
{
  /* Only HTTP 1.0 and 1.1 are supported. */
  g_return_if_fail (major == 1 && (0 <= minor && minor <= 1));

  switch (minor)
    {
    case 0:				/* HTTP 1.0 */

      /* See RFC 1945. */

      /* For now, we don't support HTTP 1.0 style keep-alive. (see RFC 2068) */
      if (header->connection_type == EVA_HTTP_CONNECTION_KEEPALIVE)
	header->connection_type = EVA_HTTP_CONNECTION_CLOSE;

      /* No Transfer-Encoding header in HTTP 1.0 */
      header->transfer_encoding_type = EVA_HTTP_TRANSFER_ENCODING_NONE;

      break;

    case 1:				/* HTTP 1.1 */
      break;
    }

  header->http_major_version = major;
  header->http_minor_version = minor;
}

void
eva_http_header_add_pragma (EvaHttpHeader *header,
                            const char    *pragma)
{
  header->pragmas = g_slist_append (header->pragmas, g_strdup (pragma));
}

void
eva_http_header_add_accepted_range (EvaHttpHeader *header,
                                    EvaHttpRange   range)
{
  EvaHttpRangeSet *rs = eva_http_range_set_new (range);
  EvaHttpRangeSet *cur_last_range = header->accepted_range_units;
  if (cur_last_range)
    {
      while (cur_last_range->next)
        cur_last_range = cur_last_range->next;
      cur_last_range->next = rs;
    }
  else
    header->accepted_range_units = rs;
}

static void
eva_http_header_finalize (GObject *object)
{
  EvaHttpHeader *header = EVA_HTTP_HEADER (object);
  eva_http_header_free_string (header, header->content_encoding);
  eva_http_header_free_string (header, header->content_type);
  eva_http_header_free_string (header, header->content_subtype);
  eva_http_header_free_string (header, header->content_charset);
  if (header->content_languages)
    g_strfreev (header->content_languages);
  while (header->content_additional)
    {
      gchar *str = header->content_additional->data;
      header->content_additional = g_slist_remove (header->content_additional, str);
      eva_http_header_free_string (header, str);
    }
  while (header->accepted_range_units)
    {
      EvaHttpRangeSet *next = header->accepted_range_units->next;
      eva_http_range_set_free (header->accepted_range_units);
      header->accepted_range_units = next;
    }
  if (header->g_error)
    g_error_free (header->g_error);
  g_free (header->unrecognized_transfer_encoding);
  g_free (header->unrecognized_content_encoding);
  if (header->header_lines)
    g_hash_table_destroy (header->header_lines);
  g_slist_foreach (header->errors, (GFunc) g_free, NULL);
  g_slist_foreach (header->pragmas, (GFunc) g_free, NULL);
  g_slist_free (header->errors);
  g_slist_free (header->pragmas);
  parent_class->finalize (object);
}

static void
eva_http_header_init (EvaHttpHeader *http_header)
{
  http_header->http_major_version = 1;
  http_header->http_minor_version = 1;
  http_header->connection_type = EVA_HTTP_CONNECTION_NONE;
  http_header->content_length = -1;
  http_header->date = -1;
  http_header->range_start = http_header->range_end = -1;
  http_header->transfer_encoding_type = EVA_HTTP_TRANSFER_ENCODING_NONE;
  http_header->content_encoding_type = EVA_HTTP_CONTENT_ENCODING_IDENTITY;
}

static void
eva_http_header_class_init (EvaHttpHeaderClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  parent_class = g_type_class_peek_parent (class);
  object_class->get_property = eva_http_header_get_property;
  object_class->set_property = eva_http_header_set_property;
  object_class->finalize = eva_http_header_finalize;
  eva_http_connection_class = g_type_class_ref (EVA_TYPE_HTTP_CONNECTION);

  g_object_class_install_property (object_class,
                                   PROP_HEADER_MAJOR_VERSION,
                                   g_param_spec_uint ("major-version",
                                                     _("Major Version"),
                                                     _("major http version"),
                                                     0, 2, 1, G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_HEADER_MINOR_VERSION,
                                   g_param_spec_uint ("minor-version",
                                                     _("Minor Version"),
                                                     _("minor http version"),
                                                     0, 10, 1, G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_HEADER_CONNECTION,
                                   g_param_spec_enum ("connection",
                                                     _("Connection-Type"),
                                                     _("type of connection"),
                                                     EVA_TYPE_HTTP_CONNECTION,
                                                     EVA_HTTP_CONNECTION_NONE,
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_HEADER_CONTENT_ENCODING,
                                   g_param_spec_enum ("content-encoding",
                                                     _("Content Encoding Type"),
                                                     _("type of content encoding"),
                                                     EVA_TYPE_HTTP_CONTENT_ENCODING,
                                                     EVA_HTTP_CONTENT_ENCODING_IDENTITY,
                                                     G_PARAM_READWRITE));
#if 0
  g_object_class_install_property (object_class,
                                   PROP_RESPONSE_CONTENT_ENCODING,
                                   g_param_spec_string ("content-encoding",
						      _("Content-Encoding"),
						      _("The method by which the content is encoded"),
						      NULL,
						      G_PARAM_READWRITE));
#endif
  g_object_class_install_property (object_class,
                                   PROP_HEADER_CONTENT_TYPE,
                                   g_param_spec_string ("content-type",
						      _("Content-Type"),
						      _("The major type of content"),
						      NULL,
						      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_HEADER_CONTENT_SUBTYPE,
                                   g_param_spec_string ("content-subtype",
						      _("Content-Subtype"),
						      _("The minor type of content"),
						      NULL,
						      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_HEADER_CONTENT_CHARSET,
                                   g_param_spec_string ("content-charset",
						      _("Content-Charset"),
						      _("The character set used for text content"),
						      NULL,
						      G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_HEADER_TRANSFER_ENCODING,
                                   g_param_spec_enum ("transfer-encoding",
                                                     _("Transfer Encoding Type"),
                                                     _("type of transfer encoding"),
                                                     EVA_TYPE_HTTP_TRANSFER_ENCODING,
                                                     EVA_HTTP_TRANSFER_ENCODING_NONE,
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_HEADER_CONNECTION_STRING,
                                   g_param_spec_string ("connection-string",
                                                        _("Connection-Type (string)"),
                                                        _("type of connection as string"),
                                                        "close",
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_HEADER_CONTENT_ENCODING_STRING,
                                   g_param_spec_string ("content-encoding-string",
                                                        _("Content Encoding-Type (string)"),
                                                        _("type of content encoding as string"),
                                                        "identity",
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_HEADER_TRANSFER_ENCODING_STRING,
                                   g_param_spec_string ("transfer-encoding-string",
                                                        _("Transfer Encoding-Type (string)"),
                                                        _("type of transfer encoding as string"),
                                                        "none",
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_HEADER_CONTENT_LENGTH,
                                   g_param_spec_int ("content-length",
                                                     _("Content Length"),
                                                     _("length of the data"),
                                                     -1, G_MAXINT, -1, G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_HEADER_RANGE_START,
                                   g_param_spec_int ("range-start",
                                                     _("Range Start"),
                                                     _("beginning of the data"),
                                                     -1, G_MAXINT, -1, G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_HEADER_RANGE_END,
                                   g_param_spec_int ("range-end",
                                                     _("Range End"),
                                                     _("end of the data"),
                                                     -1, G_MAXINT, -1, G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_HEADER_DATE,
                                   g_param_spec_int ("date",
                                                     _("Date"),
                                                     _("date of the content"),
                                                     -1, G_MAXLONG, -1, G_PARAM_READWRITE));

}

GType eva_http_header_get_type()
{
  static GType http_header_type = 0;
  if (!http_header_type)
    {
      static const GTypeInfo http_header_info =
      {
        sizeof(EvaHttpHeaderClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) eva_http_header_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (EvaHttpHeader),
        0,              /* n_preallocs */
        (GInstanceInitFunc) eva_http_header_init,
        NULL            /* value_table */
      };
      http_header_type = g_type_register_static (G_TYPE_OBJECT,
                                                 "EvaHttpHeader",
                                                 &http_header_info,
                                                 G_TYPE_FLAG_ABSTRACT);
    }
  return http_header_type;
}

/* --- foreign headers --- */
/**
 * eva_http_header_add_misc:
 * @header: the header to affect.
 * @key: a case-insensitive ASCII string for the key for this foreign header.
 *   This key should not be a standard HTTP tag.
 * @value: a case-sensitive value for that key.
 *
 * Add a raw header line to the header, with an associated value.
 */
void
eva_http_header_add_misc    (EvaHttpHeader *header,
			     const char    *key,
			     const char    *value)
{
  /* TODO: should use a case-insensitive hash function,
           instead of g_ascii_strdown()!!! */
  if (header->header_lines == NULL)
    header->header_lines = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  g_hash_table_insert (header->header_lines, g_ascii_strdown (key, -1), g_strdup (value));
}

/**
 * eva_http_header_remove_misc:
 * @header: the header to affect.
 * @key: a case-insensitive ASCII string for the key for this foreign header.
 *   This key should not be a standard HTTP tag.
 *
 * Remove a raw header line from the header.
 */
void
eva_http_header_remove_misc  (EvaHttpHeader *header,
			      const char    *key)
{
  guint len;
  char *lower;
  guint i;
  len = strlen (key);
  lower = g_alloca (len + 1);
  for (i = 0; i < len; i++)
    lower[i] = g_ascii_tolower (key[i]);
  lower[i] = 0;

  g_return_if_fail (g_hash_table_lookup (header->header_lines, lower) != NULL);
  g_hash_table_remove (header->header_lines, lower);
}

/**
 * eva_http_header_lookup_misc:
 * @header: the header to query.
 * @key: a case-insensitive ASCII string for the key for this foreign header.
 *   This key should not be a standard HTTP tag.
 * returns: value of the key.
 *
 * Lookup a miscellaneous HTTP header.  All headers that begin with X-
 * are stored in here.
 */
const char *
eva_http_header_lookup_misc  (EvaHttpHeader *header,
                              const char    *key)
{
  guint len;
  char *lower;
  guint i;

  if (header->header_lines == NULL)
    return NULL;

  len = strlen (key);
  lower = g_alloca (len + 1);
  for (i = 0; i < len; i++)
    lower[i] = g_ascii_tolower (key[i]);
  lower[i] = 0;
  return g_hash_table_lookup (header->header_lines, lower);
}

/* --- Enum type registration --- */
#include "evahttpheader.inc"    /* machine-generated enum-value tables */
#define DEFINE_ENUM_GET_TYPE_FUNC(class, lower)                                 \
GType lower ## _get_type (void)                                                 \
{                                                                               \
  static GType type = 0;                                                        \
  if (type == 0)                                                                \
    type = g_enum_register_static (#class, lower ## _enum_values);              \
  return type;                                                                  \
}
DEFINE_ENUM_GET_TYPE_FUNC(EvaHttpConnection,        eva_http_connection)
DEFINE_ENUM_GET_TYPE_FUNC(EvaHttpVerb,              eva_http_verb)
DEFINE_ENUM_GET_TYPE_FUNC(EvaHttpRange,             eva_http_range)
DEFINE_ENUM_GET_TYPE_FUNC(EvaHttpTransferEncoding,  eva_http_transfer_encoding)
DEFINE_ENUM_GET_TYPE_FUNC(EvaHttpContentEncoding,   eva_http_content_encoding)
DEFINE_ENUM_GET_TYPE_FUNC(EvaHttpStatus,            eva_http_status)
#undef DEFINE_ENUM_GET_TYPE_FUNC

/* --- Miscellaneous boxed types --- */

/* set OUT to NULL if IN is NULL,
   or SLAB_AT otherwise.  Copy IN to SLAB_AT and 
   advance SLAB_AT. */
#define MAYBE_COPY(out, in, slab_at)            \
  G_STMT_START{                                 \
    if (in == NULL)                             \
      out = NULL;                               \
    else                                        \
      {                                         \
        out = slab_at;                          \
        slab_at = g_stpcpy (slab_at, in) + 1;   \
      }                                         \
  }G_STMT_END

/* EvaHttpAuthenticate */
/**
 * eva_http_authenticate_new_unknown:
 * @challenge: the string to challenge with.
 *
 * Handling your special purpose authenticator.
 * Note that Basic and Digest authenticator parsers
 * are built-in to EVA, this is only for other types.
 *
 * It is given from the server to the client.
 * The client must response (in another message) with
 * an authorization.
 *
 * The purpose of a challenge in cryptographic systems
 * is to randomize the transaction so that an attacker cannot
 * use the same Authorization lines to get access to a machine.
 *
 * Of course, if you use an insecure transport layer, it is MUCH
 * harder to make things secure -- using SSL is recommended.
 *
 * Some WWW-Authenticate header is required
 * in a EVA_HTTP_STATUS_UNAUTHORIZED response message.
 *
 * returns: the new challenge.
 */
EvaHttpAuthenticate*
eva_http_authenticate_new_unknown (const char *auth_scheme_name,
                                   const char *realm,
                                   const char *options)
{
  guint len = sizeof (EvaHttpAuthenticate)
            + ACTUAL_LENGTH (auth_scheme_name)
            + ACTUAL_LENGTH (realm)
            + ACTUAL_LENGTH (options);
  char *at;
  EvaHttpAuthenticate *auth = g_malloc (len);
  auth->mode = EVA_HTTP_AUTH_MODE_UNKNOWN;
  auth->ref_count = 1;
  at = (char *)(auth + 1);
  MAYBE_COPY(auth->auth_scheme_name, auth_scheme_name, at);
  MAYBE_COPY(auth->realm, realm, at);
  MAYBE_COPY(auth->info.unknown.options, realm, at);
  return auth;
}

/**
 * eva_http_authenticate_new_basic:
 * @realm: the area on the site to authenticate for.
 * This is the name used by the RFC.
 *
 * Create an #EvaHttpAuthenticate for basic authentication.
 * Basic authentication is very insecure: the password
 * is merely transmitted base64 encoded.
 *
 * returns: the new challenge.
 */
EvaHttpAuthenticate *eva_http_authenticate_new_basic   (const char *realm)
{
  guint len = sizeof (EvaHttpAuthenticate)
            + ACTUAL_LENGTH (realm);
  char *at;
  EvaHttpAuthenticate *auth = g_malloc (len);
  auth->mode = EVA_HTTP_AUTH_MODE_BASIC;
  at = (char *)(auth + 1);
  auth->auth_scheme_name = "Basic";
  auth->ref_count = 1;
  MAYBE_COPY (auth->realm, realm, at);
  return auth;
}

/**
 * eva_http_authenticate_new_digest:
 * @realm: the area on the site to authenticate for.
 * @domain:
 * @nonce: value hashed into the digest to keep it secure against replay attacks.
 * @opaque: value which must be sent back to the server,
 * which the client does not interpret.
 * @algorithm: digest algorithm.  Only MD5 is supported for now,
 * and is equivalent to specifying NULL.
 *
 * Create an #EvaHttpAuthenticate for Digest authentication.
 * Digest authentication improves Basic authentication by
 * using a hash of the password.
 *
 * returns: the new challenge.
 */
EvaHttpAuthenticate *eva_http_authenticate_new_digest  (const char *realm,
                                                        const char *domain,
                                                        const char *nonce,
                                                        const char *opaque,
                                                        const char *algorithm)
{
  gboolean is_md5 = algorithm == NULL || strcmp (algorithm, "MD5") == 0;
  guint len = sizeof (EvaHttpAuthenticate)
            + ACTUAL_LENGTH (realm)
            + ACTUAL_LENGTH (domain)
            + ACTUAL_LENGTH (nonce)
            + ACTUAL_LENGTH (opaque)
            + (is_md5 ? 0 : (strlen (algorithm) + 1));
  EvaHttpAuthenticate *auth = g_malloc (len);
  char *at;
  auth->ref_count = 1;
  auth->mode = EVA_HTTP_AUTH_MODE_DIGEST;
  auth->auth_scheme_name = "Digest";
  at = (char*)(auth + 1);
  auth->realm = at;
  at = g_stpcpy (at, realm) + 1;
  MAYBE_COPY (auth->realm, realm, at);
  MAYBE_COPY (auth->info.digest.domain, domain, at);
  MAYBE_COPY (auth->info.digest.nonce, nonce, at);
  MAYBE_COPY (auth->info.digest.opaque, opaque, at);
  auth->info.digest.algorithm = is_md5 ? NULL : strcpy (at, algorithm);
  return auth;
}

#if 0
static EvaHttpAuthenticate *
eva_http_authenticate_copy (const EvaHttpAuthenticate *auth)
{
  switch (auth->mode)
    {
    case EVA_HTTP_AUTH_MODE_UNKNOWN:
      return eva_http_authenticate_new_unknown (auth->auth_scheme_name,
                                                auth->realm,
                                                auth->info.unknown.options);
    case EVA_HTTP_AUTH_MODE_BASIC:
      return eva_http_authenticate_new_basic (auth->realm);

    case EVA_HTTP_AUTH_MODE_DIGEST:
      return eva_http_authenticate_new_digest (auth->realm,
                                               auth->info.digest.domain,
                                               auth->info.digest.nonce,
                                               auth->info.digest.opaque,
                                               auth->info.digest.algorithm);
    default:
      g_return_val_if_reached (NULL);
    }
}
#endif

EvaHttpAuthenticate *
eva_http_authenticate_ref (EvaHttpAuthenticate *auth)
{
  g_return_val_if_fail (auth->ref_count != 0, auth);
  ++(auth->ref_count);
  return auth;
}

void
eva_http_authenticate_unref (EvaHttpAuthenticate *auth)
{
  g_return_if_fail (auth->ref_count != 0);
  if (--(auth->ref_count) == 0)
    {
      g_free (auth);
    }
}

/* EvaHttpAuthorization */
/**
 * eva_http_authorization_new_unknown:
 * @auth_scheme_name: name of the authentification scheme.
 *
 * Arbitrary authorization response.
 *
 * We recommend you use eva_http_authorization_new_digest()
 * instead, or eva_http_authorization_new_basic() if that's all
 * the server can do.
 *
 * returns: the response to the authentication request.
 */
EvaHttpAuthorization *eva_http_authorization_new_unknown (const char *auth_scheme_name,
                                                          const char *response)
{
  guint len = sizeof (EvaHttpAuthorization)
            + ACTUAL_LENGTH (auth_scheme_name)
            + ACTUAL_LENGTH (response);
  char *at;
  EvaHttpAuthorization *auth = g_malloc (len);
  at = (char *)(auth + 1);
  auth->mode = EVA_HTTP_AUTH_MODE_UNKNOWN;
  MAYBE_COPY (auth->auth_scheme_name, auth_scheme_name, at);
  MAYBE_COPY (auth->info.unknown.response, response, at);
  auth->ref_count = 1;
  return auth;
}

/**
 * eva_http_authorization_new_basic:
 * @realm: the area on the site to authenticate for.
 * @user: name of the account to authenticate for.
 * @password: password of the account to authenticate for.
 *
 * Basic HTTP authentication response.
 * Not very secure because your password is provided plain-text.
 *
 * returns: the response to the authentication request.
 */
EvaHttpAuthorization *eva_http_authorization_new_basic   (const char *user,
                                                          const char *password)
{
  guint len = sizeof (EvaHttpAuthorization)
            + ACTUAL_LENGTH (user)
            + ACTUAL_LENGTH (password);
  char *at;
  EvaHttpAuthorization *auth = g_malloc (len);
  at = (char *)(auth + 1);
  auth->mode = EVA_HTTP_AUTH_MODE_BASIC;
  auth->auth_scheme_name = "Basic";
  MAYBE_COPY (auth->info.basic.user, user, at);
  MAYBE_COPY (auth->info.basic.password, password, at);
  auth->ref_count = 1;
  return auth;
}
/**
 * eva_http_authorization_new_digest:
 * @realm: the area on the site to authenticate for.
 * @domain:
 * @nonce: value hashed into the digest to keep it secure against replay attacks.
 * @opaque: value which must be sent back to the server,
 * which the client does not interpret.
 * @algorithm: digest algorithm.  Only MD5 is supported for now,
 * and is equivalent to specifying NULL.
 * @user: name of the account to authenticate for.
 * @password: password of the account to authenticate for. [unnecessary if you have the response digest]
 * This value is NOT transmitted to the remote host.
 * @response_digest: the securely-hashed response.   If NULL, Eva will compute the response.
 * @entity_digest: the securely-hashed information about your POST content.
 *
 * Create a new Digest-based authentication response.
 * Many of the parameters must match the Authenticate header.
 *
 * The response_digest is a value computed from nonce, user, and password.
 * The security of Digest authentication derives from the difficulty of
 * computing the password from the digest, since the password itself is
 * not transmitted cleartext.
 *
 * returns: the response to the authentication request.
 */
EvaHttpAuthorization *eva_http_authorization_new_digest  (const char *realm,
                                                          const char *domain,
                                                          const char *nonce,
                                                          const char *opaque,
                                                          const char *algorithm,
                                                          const char *user,
                                                          const char *password,
                                                          const char *response_digest,
                                                          const char *entity_digest)
{
  /* NB: nonce, response_digest, entity_digest are all allocated separately */
  gboolean is_md5 = algorithm == NULL || strcmp (algorithm, "MD5") == 0;
  guint len = sizeof (EvaHttpAuthorization)
            + ACTUAL_LENGTH (realm)
            + ACTUAL_LENGTH (domain)
            + ACTUAL_LENGTH (opaque)
            + ACTUAL_LENGTH (user)
            + ACTUAL_LENGTH (password)
            + (is_md5 ? 0 : (strlen (algorithm) + 1));
  char *at;
  EvaHttpAuthorization *auth = g_malloc (len);
  at = (char *)(auth + 1);
  auth->mode = EVA_HTTP_AUTH_MODE_DIGEST;
  auth->auth_scheme_name = "Digest";
  MAYBE_COPY (auth->info.digest.realm, realm, at);
  MAYBE_COPY (auth->info.digest.domain, domain, at);
  MAYBE_COPY (auth->info.digest.opaque, opaque, at);
  MAYBE_COPY (auth->info.digest.user, user, at);
  MAYBE_COPY (auth->info.digest.password, password, at);
  if (is_md5)
    auth->info.digest.algorithm = NULL;
  else
    auth->info.digest.algorithm = strcpy (at, algorithm);
  auth->info.digest.response_digest = g_strdup (response_digest);
  auth->info.digest.entity_digest = g_strdup (entity_digest);
  auth->ref_count = 1;
  return auth;
}

/**
 * eva_http_authorization_new_respond:
 * @auth: the authentication request to respond to (given as a challenge by the server).
 * @user: the username of the account (or whatever) to use.
 * @password: password of the account to authenticate for.
 * @error: optional place to put the error if something goes wrong.
 *
 * Attempt to use the given user/password pair as authentication information for
 * the given request.
 *
 * returns: the new response, or NULL if something goes wrong.
 */
EvaHttpAuthorization *
eva_http_authorization_new_respond (const EvaHttpAuthenticate *auth,
                                    const char *user,
                                    const char *password,
                                    GError    **error)
{
  switch (auth->mode)
    {
    case EVA_HTTP_AUTH_MODE_UNKNOWN:
      g_set_error (error,
                   EVA_G_ERROR_DOMAIN,
                   EVA_ERROR_UNKNOWN,
                   "cannot response to unknown authentication scheme %s",
                   auth->auth_scheme_name);
      return NULL;

    case EVA_HTTP_AUTH_MODE_BASIC:
      return eva_http_authorization_new_basic (user, password);

    case EVA_HTTP_AUTH_MODE_DIGEST:
      return eva_http_authorization_new_digest (auth->realm,
                                                auth->info.digest.domain,
                                                auth->info.digest.nonce,
                                                auth->info.digest.opaque,
                                                auth->info.digest.algorithm,
                                                user,
                                                password,
                                                NULL,
                                                NULL);

    default:
      g_return_val_if_reached (NULL);
    }
}

void
eva_http_authorization_set_nonce   (EvaHttpAuthorization *auth,
                                    const char           *nonce)
{
  char *copy;
  g_return_if_fail (auth->mode == EVA_HTTP_AUTH_MODE_DIGEST);
  g_return_if_fail (auth->info.digest.password != NULL);
  copy = g_strdup (nonce);
  g_free (auth->info.digest.nonce);
  auth->info.digest.nonce = copy;
  g_free (auth->info.digest.response_digest);
  auth->info.digest.response_digest = NULL;
}

EvaHttpAuthorization *
eva_http_authorization_copy (const EvaHttpAuthorization *auth)
{
  switch (auth->mode)
    {
    case EVA_HTTP_AUTH_MODE_UNKNOWN:
      return eva_http_authorization_new_unknown (auth->auth_scheme_name,
                                                 auth->info.unknown.response);
    case EVA_HTTP_AUTH_MODE_BASIC:
      return eva_http_authorization_new_basic (auth->info.basic.user,
                                               auth->info.basic.password);
    case EVA_HTTP_AUTH_MODE_DIGEST:
      return eva_http_authorization_new_digest  (auth->info.digest.realm,
                                                 auth->info.digest.domain,
                                                 auth->info.digest.nonce,
                                                 auth->info.digest.opaque,
                                                 auth->info.digest.algorithm,
                                                 auth->info.digest.user,
                                                 auth->info.digest.password,
                                                 auth->info.digest.response_digest,
                                                 auth->info.digest.entity_digest);
    default:
      g_return_val_if_reached (NULL);
    }
}

EvaHttpAuthorization *
eva_http_authorization_ref (EvaHttpAuthorization *auth)
{
  g_return_val_if_fail (auth->ref_count != 0, auth);
  ++(auth->ref_count);
  return auth;
}

void
eva_http_authorization_unref (EvaHttpAuthorization *auth)
{
  g_return_if_fail (auth->ref_count != 0);
  if (--(auth->ref_count) == 0)
    {
      if (auth->mode == EVA_HTTP_AUTH_MODE_DIGEST)
        {
          g_free (auth->info.digest.nonce);
          g_free (auth->info.digest.response_digest);
          g_free (auth->info.digest.entity_digest);
        }
      g_free (auth);
    }
}


/* EvaHttpResponseCacheDirective */
/**
 *
 * eva_http_response_cache_directive_new:
 * Create a new cache respnse directive.
 * @Returns: the new cache response directive.
 */
EvaHttpResponseCacheDirective *
eva_http_response_cache_directive_new (void)
{
  EvaHttpResponseCacheDirective *directive =
    g_new0 (EvaHttpResponseCacheDirective, 1);
  directive->is_public = 1;
  return directive;
}

/**
 * eva_http_response_cache_directive_set_private_name:
 * @directive: the cache directive to affect.
 * @name: new qualifier for how the private data is allowed to be used.
 * @name_len: length of name string
 *
 * Intended for more detailed specification of private cache data.
 */
void
eva_http_response_cache_directive_set_private_name (
  EvaHttpResponseCacheDirective *cd,
  const char                    *name,
  gsize                          name_len)
{
  char *n = g_strndup (name, name_len);
  g_free (cd->private_name);
  cd->private_name = n;
}

/**
 * eva_http_response_cache_directive_set_no_cache_name:
 * @directive: the cache directive to affect.
 * @name: new qualifier for how the no-cache data is allowed to be used.
 *
 * Intended for more detailed specification of no-cache data,
 * which is never supposed to be cached.
 *
 * [RFC 2616, 14.9.1]
 * If the no-cache directive does specify one or more field-names,
 * then a cache MAY use the response to satisfy a subsequent request,
 * subject to any other restrictions on caching. However, the
 * specified field-name(s) MUST NOT be sent in the response to a
 * subsequent request without successful revalidation with the origin
 * server. This allows an origin server to prevent the re-use of
 * certain header fields in a response, while still allowing caching
 * of the rest of the response.
 *
 * good luck.
 */
void
eva_http_response_cache_directive_set_no_cache_name (
  EvaHttpResponseCacheDirective *cd,
  const char                    *name,
  gsize                          name_len)
{
  char *n = g_strndup (name, name_len);
  g_free (cd->no_cache_name);
  cd->no_cache_name = n;
}

/**
 * eva_http_response_cache_directive_free:
 * @directive: cache-directive to de-allocate.
 *
 * Deallocate a EvaHttpResponseCacheDirective.
 */
void
eva_http_response_cache_directive_free (EvaHttpResponseCacheDirective *directive)
{
  g_free (directive->no_cache_name);
  g_free (directive->private_name);
  g_free (directive);
}

static EvaHttpResponseCacheDirective *
eva_http_response_cache_directive_copy (EvaHttpResponseCacheDirective *directive)
{
  EvaHttpResponseCacheDirective *rv;

  rv = g_memdup (directive, sizeof (*directive));
  rv->no_cache_name = g_strdup (rv->no_cache_name);
  rv->private_name = g_strdup (rv->private_name);
  return rv;
}


/* EvaHttpRequestCacheDirective */
/**
 * 
 * eva_http_request_cache_directive_new:
 * Create a new cache respnse directive.
 * returns: the new cache request directive.
 */
EvaHttpRequestCacheDirective *
eva_http_request_cache_directive_new (void)
{
  return g_new0 (EvaHttpRequestCacheDirective, 1);
}

/**
 * eva_http_request_cache_directive_free:
 * @directive: cache-directive to de-allocate.
 *
 * Deallocate a EvaHttpRequestCacheDirective.
 */
void
eva_http_request_cache_directive_free (EvaHttpRequestCacheDirective *directive)
{
  g_free (directive);
}

static EvaHttpRequestCacheDirective *
eva_http_request_cache_directive_copy (EvaHttpRequestCacheDirective *directive)
{
  return g_memdup (directive, sizeof (*directive));
}


/* EvaHttpCharSet */
/**
 * eva_http_char_set_new:
 * @charset_name: name of the character set.
 * @quality: quality from 0 to 1, or -1 if no quality flag was given.
 *
 * Allocate a single EvaHttpCharSet preference.
 * You may wish to build a list of these.
 *
 * returns: the new character-set.
 */
EvaHttpCharSet *
eva_http_char_set_new (const char *charset_name,
		       gfloat      quality)
{
  EvaHttpCharSet *char_set = g_new (EvaHttpCharSet, 1);
  char_set->charset_name = g_strdup (charset_name);
  char_set->quality = quality;
  char_set->next = NULL;
  return char_set;
}

static EvaHttpCharSet *
eva_http_char_set_copy (EvaHttpCharSet *char_set)
{
  return eva_http_char_set_new (char_set->charset_name,
				char_set->quality);
}

/**
 * eva_http_char_set_free:
 * @char_set: character set to free.
 *
 * Deallocate a EvaHttpCharSet.
 */
void
eva_http_char_set_free(EvaHttpCharSet *char_set)
{
  g_free (char_set->charset_name);
  g_free (char_set);
}


/* EvaHttpCookie */
/**
 * eva_http_cookie_new:
 * @key: token used to identify the cookie.
 * @value: information associated with the cookie.
 * @path: applicable area in server directory structure for this cookie.
 * @domain: domain names this cookie applies to.
 * @expire_date: when this cookie should be discarded.
 * @comment: miscellaneous information about this cookie.
 * @max_age: maximum number of seconds that this cookie should be retained.
 *
 * Allocate a cookie.  Once allocated, it cannot be changed.
 *
 * Cookies are a mechanism for tracking users.
 * The remote host sends a Set-Cookie directive which,
 * should be client choose to accept it, may be sent with
 * subsequent relevant requests.
 *
 * NOTE: EVA does not automatically handle cookies.
 * That is up to the application.
 *
 * TODO: EVA should have a cookie database mechanism.
 *
 * See RFC 2964 and 2965.
 *
 * returns: the newly allocated cookie.
 */
EvaHttpCookie  *
eva_http_cookie_new              (const char     *key,
				  const char     *value,
				  const char     *path,
				  const char     *domain,
				  const char     *expire_date,
				  const char     *comment,
				  int             max_age)
{
#define ACTUAL_LENGTH(str)	((str) ? (strlen (str) + 1) : 0)
  guint alloc_length = sizeof (EvaHttpCookie)
                     + ACTUAL_LENGTH (key)
                     + ACTUAL_LENGTH (value)
                     + ACTUAL_LENGTH (path)
                     + ACTUAL_LENGTH (domain)
                     + ACTUAL_LENGTH (expire_date)
                     + ACTUAL_LENGTH (comment);
  guint at = sizeof (EvaHttpCookie);
  char *raw = g_new (char, alloc_length);
  EvaHttpCookie *rv = (EvaHttpCookie *) raw;
  rv->max_age = max_age;
#define INIT_RV_STRING(name)			\
  G_STMT_START{					\
    if (name == NULL)				\
      rv->name = NULL;				\
    else					\
      {						\
	rv->name = strcpy (raw + at, name);	\
	at += strlen (name) + 1;		\
      }						\
  }G_STMT_END
  INIT_RV_STRING (key);
  INIT_RV_STRING (value);
  INIT_RV_STRING (path);
  INIT_RV_STRING (domain);
  INIT_RV_STRING (expire_date);
  INIT_RV_STRING (comment);
#if 0		/* uh, we "reuse"/"abuse" these below in MediaTypeSet */
#undef ACTUAL_LENGTH
#undef INIT_RV_STRING
#endif
  g_assert (at == alloc_length);
  rv->version = 0;
  rv->secure = FALSE;
  return rv;
}

/**
 * eva_http_cookie_copy:
 * @orig: the cookie to copy.
 *
 * Copy a cookie.
 *
 * returns: the new cookie.
 */
EvaHttpCookie  *
eva_http_cookie_copy (const EvaHttpCookie *orig)
{
  EvaHttpCookie *rv = eva_http_cookie_new (orig->key,
			                   orig->value,
			                   orig->path,
			                   orig->domain,
			                   orig->expire_date,
			                   orig->comment,
			                   orig->max_age);
  rv->secure = orig->secure;
  rv->version = orig->version;
  return rv;
}

/**
 * eva_http_cookie_free:
 * @orig: the cookie to deallocate.
 *
 * Free the memory associated with the cookie.
 */
void
eva_http_cookie_free (EvaHttpCookie *orig)
{
  g_return_if_fail (orig != NULL);
  g_free (orig);
}

/* EvaHttpContentEncodingSet */
/**
 * eva_http_content_encoding_set_new:
 * @encoding: the encoding to list a preference for.
 * @quality: relative preference for this encoding.
 * A value of -1 means that the quality was omitted,
 * which means it should be treated as 1.
 *
 * Allocate a new node in a EvaHttpContentEncodingSet list.
 *
 * returns: the new encoding node.
 */
EvaHttpContentEncodingSet  *
eva_http_content_encoding_set_new (EvaHttpContentEncoding encoding,
			           gfloat          quality)
{
  EvaHttpContentEncodingSet *set = g_new (EvaHttpContentEncodingSet, 1);
  set->encoding = encoding;
  set->quality = quality;
  set->next = NULL;
  return set;
}

static EvaHttpContentEncodingSet *
eva_http_content_encoding_set_copy (EvaHttpContentEncodingSet *set)
{
  return eva_http_content_encoding_set_new (set->encoding, set->quality);
}

/**
 * eva_http_content_encoding_set_free:
 * @encoding_set: the encoding set to deallocate.
 *
 * Deallocate the encoding.
 */
void
eva_http_content_encoding_set_free(EvaHttpContentEncodingSet *encoding_set)
{
  g_return_if_fail (encoding_set != NULL);
  g_free (encoding_set);
}

/* EvaHttpTransferEncodingSet */
/**
 * eva_http_transfer_encoding_set_new:
 * @encoding: the encoding to list a preference for.
 * @quality: relative preference for this encoding.
 * A value of -1 means that the quality was omitted,
 * which means it should be treated as 1.
 *
 * Allocate a new node in a EvaHttpTransferEncodingSet list.
 *
 * returns: the new encoding node.
 */
EvaHttpTransferEncodingSet  *
eva_http_transfer_encoding_set_new (EvaHttpTransferEncoding encoding,
			            gfloat          quality)
{
  EvaHttpTransferEncodingSet *set = g_new (EvaHttpTransferEncodingSet, 1);
  set->encoding = encoding;
  set->quality = quality;
  set->next = NULL;
  return set;
}

static EvaHttpTransferEncodingSet *
eva_http_transfer_encoding_set_copy (EvaHttpTransferEncodingSet *set)
{
  return eva_http_transfer_encoding_set_new (set->encoding, set->quality);
}

/**
 * eva_http_transfer_encoding_set_free:
 * @encoding_set: the encoding set to deallocate.
 *
 * Deallocate the encoding (a single node in the list).
 */
void
eva_http_transfer_encoding_set_free(EvaHttpTransferEncodingSet *encoding_set)
{
  g_return_if_fail (encoding_set != NULL);
  g_free (encoding_set);
}

/* EvaHttpLanguageSet */
/**
 * eva_http_language_set_new:
 * @language: the human language code to list a preference for.
 * @quality: relative preference for this encoding.
 * A value of -1 means that the quality was omitted,
 * which means it should be treated as 1.
 *
 * Allocate a new node in a EvaHttpLanguageSet list.
 * Though any ASCII string is basically allowed,
 * a two letter language code (en, de, pl, it, etc)
 * is the usual start of the language name;
 * a two letter country code (US, UK, DE, etc)
 * is the second more optional part, like "en-US".
 *
 * returns: the new language node.
 */
EvaHttpLanguageSet *
eva_http_language_set_new       (const char *language,
			         gfloat      quality)
{
  /* ugh, these macros come from eva_http_cookie_new above */
  guint alloc_length = sizeof (EvaHttpLanguageSet)
                     + strlen (language) + 1;
  EvaHttpLanguageSet *rv = g_malloc (alloc_length);
  char *mem_at = (char*)(rv + 1);
  rv->quality = quality;
  rv->next = NULL;
  rv->language = mem_at;
  strcpy (rv->language, language);
  return rv;
}

static EvaHttpLanguageSet *
eva_http_language_set_copy(EvaHttpLanguageSet *set)
{
  return eva_http_language_set_new (set->language,
                                    set->quality);
}

/**
 * eva_http_language_set_free:
 * @set: the language set node to deallocate.
 *
 * Deallocate the node in the language-set.
 */
void
eva_http_language_set_free(EvaHttpLanguageSet *set)
{
  g_return_if_fail (set != NULL);
  g_free (set);
}


/* EvaHttpMediaType */
/**
 * eva_http_media_type_set_new:
 * @type: major type of the media to allow, like "text" or "image".
 *      An asterisk can be used to allow any major type.
 * @subtype: format of the media to allow, like "html", "plain" or "jpeg".
 *      An asterisk can be used to allow any format.
 * @quality: relative preference for this encoding. -1 means "not specified", which has a default of 1.
 *
 * Allocate a new node in a EvaHttpMediaTypeSet list.
 *
 * returns: the newly allocated node.
 */
EvaHttpMediaTypeSet *
eva_http_media_type_set_new (const char *type,
			     const char *subtype,
			     gfloat      quality)
{
  /* ugh, these macros come from eva_http_cookie_new above */
  guint alloc_length = sizeof (EvaHttpMediaTypeSet)
                     + ACTUAL_LENGTH (type)
                     + ACTUAL_LENGTH (subtype);
  guint at = sizeof (EvaHttpMediaTypeSet);
  char *raw = g_new (char, alloc_length);
  EvaHttpMediaTypeSet *rv = (EvaHttpMediaTypeSet *) raw;
  rv->quality = quality;
  rv->next = NULL;
  INIT_RV_STRING (type);
  INIT_RV_STRING (subtype);
  g_assert (at == alloc_length);
  return rv;
}

static EvaHttpMediaTypeSet *
eva_http_media_type_set_copy (EvaHttpMediaTypeSet *set)
{
  return eva_http_media_type_set_new (set->type, set->subtype,
				      set->quality);
}

/**
 * eva_http_media_type_set_free:
 * @set: the media type set to deallocate.
 *
 * Free the memory associated with the media-type-set node.
 */
void
eva_http_media_type_set_free (EvaHttpMediaTypeSet *set)
{
  g_return_if_fail (set != NULL);
  g_free (set);
}

/* EvaHttpRangeSet */
/**
 * eva_http_range_set_new:
 * @range_type: allocate a new node in a allowable range units list.
 *
 * Allocate a node telling what units are available for
 * specifying partial content.
 *
 * returns: the newly allocated node.
 */
EvaHttpRangeSet *
eva_http_range_set_new (EvaHttpRange range_type)
{
  EvaHttpRangeSet *rv = g_new (EvaHttpRangeSet, 1);
  rv->range_type = range_type;
  rv->next = NULL;
  return rv;
}

static EvaHttpRangeSet *
eva_http_range_set_copy (EvaHttpRangeSet *orig)
{
  return eva_http_range_set_new (orig->range_type);
}

/**
 * eva_http_range_set_free:
 * @set: range-unit node to deallocate.
 *
 * Free the memory associated with the node in the range-set.
 */
void
eva_http_range_set_free(EvaHttpRangeSet *set)
{
  g_return_if_fail (set != NULL);
  g_free (set);
}

#define _DEFINE_BOXED_GET_TYPE(Class, class, _copy, _free)	           \
GType									   \
class ## _get_type (void)						   \
{									   \
  static GType type = 0;						   \
  if (type == 0)							   \
    type = g_boxed_type_register_static (#Class,			   \
                                         (GBoxedCopyFunc) class ## _copy,  \
                                         (GBoxedFreeFunc) class ## _free); \
  return type;								   \
}
#define DEFINE_BOXED_GET_TYPE(Class, class) _DEFINE_BOXED_GET_TYPE(Class,class,_copy,_free)
#define DEFINE_REFCOUNTED_BOXED_GET_TYPE(Class, class) _DEFINE_BOXED_GET_TYPE(Class,class,_ref,_unref)
DEFINE_REFCOUNTED_BOXED_GET_TYPE (EvaHttpAuthenticate, eva_http_authenticate)
DEFINE_REFCOUNTED_BOXED_GET_TYPE (EvaHttpAuthorization, eva_http_authorization)
DEFINE_BOXED_GET_TYPE (EvaHttpResponseCacheDirective, 
		       eva_http_response_cache_directive)
DEFINE_BOXED_GET_TYPE (EvaHttpRequestCacheDirective, 
		       eva_http_request_cache_directive)
DEFINE_BOXED_GET_TYPE (EvaHttpCharSet, eva_http_char_set)
DEFINE_BOXED_GET_TYPE (EvaHttpCookie, eva_http_cookie)
DEFINE_BOXED_GET_TYPE (EvaHttpLanguageSet, eva_http_language_set)
DEFINE_BOXED_GET_TYPE (EvaHttpContentEncodingSet, eva_http_content_encoding_set)
DEFINE_BOXED_GET_TYPE (EvaHttpTransferEncodingSet, eva_http_transfer_encoding_set)
DEFINE_BOXED_GET_TYPE (EvaHttpMediaTypeSet, eva_http_media_type_set)
DEFINE_BOXED_GET_TYPE (EvaHttpRangeSet, eva_http_range_set)

#undef DEFINE_BOXED_GET_TYPE

/* EvaHttpHeader public methods */

