#include "evaxmlformat.h"
#include "evaxmlvaluereader.h"
#include "evaxmlvaluewriter.h"

#define ATOMIC_READ_SIZE 4096

/*
 *
 * EvaXmlValueRequest
 *
 */

typedef EvaValueRequestClass       EvaXmlValueRequestClass;
typedef struct _EvaXmlValueRequest EvaXmlValueRequest;

GType eva_xml_value_request_get_type (void) G_GNUC_CONST;

#define EVA_TYPE_XML_VALUE_REQUEST (eva_xml_value_request_get_type ())
#define EVA_XML_VALUE_REQUEST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
			       EVA_TYPE_XML_VALUE_REQUEST, \
			       EvaXmlValueRequest))
#define EVA_IS_XML_VALUE_REQUEST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_XML_VALUE_REQUEST))

struct _EvaXmlValueRequest
{
  EvaValueRequest value_request;

  EvaStream *stream;
  EvaXmlValueReader *xml_value_reader;
};

static GObjectClass *eva_xml_value_request_parent_class = NULL;

/* EvaXmlValueFunc */
static void
handle_value (const GValue *value, gpointer user_data)
{
  EvaXmlValueRequest *request = EVA_XML_VALUE_REQUEST (user_data);
  EvaStream *stream = request->stream;
  GError *error = NULL;

  g_return_if_fail (value);
  g_return_if_fail (stream);
  g_return_if_fail (G_VALUE_TYPE (value));

  g_value_init (&request->value_request.value, G_VALUE_TYPE (value));
  g_value_copy (value, &request->value_request.value);
  if (!eva_io_read_shutdown (EVA_IO (stream), &error))
    {
      if (error)
	eva_request_set_error (request, error);
    }
  eva_request_done (request);
}

static gboolean
handle_stream_is_readable (EvaIO *io, gpointer user_data)
{
  EvaXmlValueRequest *request = EVA_XML_VALUE_REQUEST (user_data);
  EvaStream *stream = request->stream;
  GError *error = NULL;
  char buf[ATOMIC_READ_SIZE];
  guint num_read;

  g_return_val_if_fail (stream == EVA_STREAM (io), FALSE);

  num_read = eva_stream_read (stream, buf, ATOMIC_READ_SIZE, &error);
  if (error)
    goto ERROR;
  if (num_read == 0)
    return eva_stream_get_is_readable (stream);

  if (!eva_xml_value_reader_input (request->xml_value_reader,
				   buf,
				   num_read,
				   &error))
    goto ERROR;

  return TRUE;

ERROR:
  eva_request_set_error (request, error);
  eva_request_done (request);
  eva_io_read_shutdown (EVA_IO (stream), NULL);
  return FALSE;
}

static gboolean
handle_stream_shutdown_read (EvaIO *io, gpointer user_data)
{
  EvaXmlValueRequest *request = EVA_XML_VALUE_REQUEST (user_data);
  EvaStream *stream = request->stream;

  g_return_val_if_fail (stream == EVA_STREAM (io), FALSE);

  /* If we're not done or cancelled, a shutdown is an error as far as
   * whoever made the original request is concerned.
   */
  if (!eva_request_get_is_done (request) &&
      !eva_request_get_is_cancelled (request))
    {
      GError *error;
      g_return_val_if_fail (eva_request_get_error (request) == NULL, FALSE);
      g_return_val_if_fail
	(G_VALUE_TYPE (&request->value_request.value) == G_TYPE_INVALID, FALSE);
      error = g_error_new (EVA_G_ERROR_DOMAIN,
			   0, /* TODO */
			   "premature shutdown of input XML stream");
      eva_request_set_error (request, error);
    }
  return FALSE;
}

static void
handle_stream_is_readable_destroy (gpointer user_data)
{
  EvaXmlValueRequest *request = EVA_XML_VALUE_REQUEST (user_data);
  EvaStream *stream = request->stream;

  g_return_if_fail (stream);
  g_object_unref (stream);
  request->stream = NULL;

  /* Get rid of reference we added for eva_io_trap_readable. */
  g_object_unref (request);
}

/* EvaRequest methods. */

static void
eva_xml_value_request_cancelled (EvaRequest *request_parent)
{
  EvaXmlValueRequest *request = EVA_XML_VALUE_REQUEST (request_parent);
  EvaStream *stream = request->stream;

  g_return_if_fail (stream);
  eva_io_read_shutdown (EVA_IO (stream), NULL);
  eva_request_mark_is_cancelled (request);
}

static void
eva_xml_value_request_start (EvaRequest *request_parent)
{
  EvaXmlValueRequest *request = EVA_XML_VALUE_REQUEST (request_parent);
  EvaStream *stream = request->stream;

  g_return_if_fail (!eva_request_get_is_running (request));
  g_return_if_fail (!eva_request_get_is_cancelled (request));
  g_return_if_fail (!eva_request_get_is_done (request));
  g_return_if_fail (request->xml_value_reader);
  g_return_if_fail (stream);

  /* This reference is on behalf of the stream's read hook. */
  g_object_ref (request);
  eva_io_trap_readable (EVA_IO (stream),
			handle_stream_is_readable,
			handle_stream_shutdown_read,
			request,
			handle_stream_is_readable_destroy);
  eva_request_mark_is_running (request);
}

/* GObject methods. */

static void
eva_xml_value_request_finalize (GObject *object)
{
  EvaXmlValueRequest *request = EVA_XML_VALUE_REQUEST (object);

  if (request->stream)
    g_object_unref (request->stream);
  if (request->xml_value_reader)
    eva_xml_value_reader_free (request->xml_value_reader);

  (*eva_xml_value_request_parent_class->finalize) (object);
}

static void
eva_xml_value_request_class_init (EvaRequestClass *request_class)
{
  eva_xml_value_request_parent_class =
    g_type_class_peek_parent (request_class);
  G_OBJECT_CLASS (request_class)->finalize = eva_xml_value_request_finalize;
  request_class->start = eva_xml_value_request_start;
  request_class->cancelled = eva_xml_value_request_cancelled;
}

GType
eva_xml_value_request_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0))
    {
      static const GTypeInfo type_info =
	{
	  sizeof (EvaXmlValueRequestClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) eva_xml_value_request_class_init,
	  NULL,		/* class_finalize */
	  NULL,		/* class_data */
	  sizeof (EvaXmlValueRequest),
	  0,		/* n_preallocs */
	  (GInstanceInitFunc) NULL,
	  NULL		/* value_table */
	};
      type = g_type_register_static (EVA_TYPE_VALUE_REQUEST,
				     "EvaXmlValueRequest",
				     &type_info,
				     0);
    }
  return type;
}

/*
 *
 * EvaXmlFormat
 *
 */

static GObjectClass *eva_xml_format_parent_class = NULL;

/*
 * EvaStorageFormat methods.
 */

static EvaValueRequest *
eva_xml_format_deserialize (EvaStorageFormat  *storage_format,
			    EvaStream         *stream,
			    GType              value_type,
			    GError           **error)
{
  EvaXmlFormat *xml_format = EVA_XML_FORMAT (storage_format);
  EvaGtypeLoader *type_loader = xml_format->type_loader;
  EvaXmlValueRequest *request;

  (void) error;

  if (type_loader == NULL)
    type_loader = eva_gtype_loader_default ();
  else
    eva_gtype_loader_ref (type_loader);

  request = g_object_new (EVA_TYPE_XML_VALUE_REQUEST, NULL);
  g_return_val_if_fail (request, NULL);
  eva_request_mark_is_cancellable (request);

  request->stream = stream;
  g_object_ref (stream);

  /* The EvaXmlValueReader can only call us back when we feed data to
   * it, so no request reference needed here.
   */
  request->xml_value_reader =
    eva_xml_value_reader_new (type_loader,
			      value_type,
			      handle_value,
			      request,
			      NULL);		/* value_func_destroy */
  eva_gtype_loader_unref (type_loader);

  return EVA_VALUE_REQUEST (request);
}

static EvaStream *
eva_xml_format_serialize (EvaStorageFormat  *format,
			  const GValue      *value,
			  GError           **error)
{
  (void) format;
  (void) error;
  return EVA_STREAM (eva_xml_value_writer_new (value));
}

/*
 * GObject methods.
 */

static void
eva_xml_format_finalize (GObject *object)
{
  EvaXmlFormat *self = EVA_XML_FORMAT (object);

  if (self->type_loader)
    eva_gtype_loader_unref (self->type_loader);

  (*eva_xml_format_parent_class->finalize) (object);
}

static void
eva_xml_format_class_init (GObjectClass *object_class)
{
  eva_xml_format_parent_class = g_type_class_peek_parent (object_class);
  object_class->finalize = eva_xml_format_finalize;
}

static void
eva_xml_format_interface_init (EvaStorageFormatIface *iface, gpointer unused)
{
  (void) unused;
  iface->serialize = eva_xml_format_serialize;
  iface->deserialize = eva_xml_format_deserialize;
}

GType
eva_xml_format_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0))
    {
      static const GInterfaceInfo storage_format_info =
	{
	  (GInterfaceInitFunc) eva_xml_format_interface_init,
	  NULL,			/* interface_finalize */
	  NULL			/* interface_data */
	};
      static const GTypeInfo info =
	{
	  sizeof (EvaXmlFormatClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) eva_xml_format_class_init,
	  NULL,		/* class_finalize */
	  NULL,		/* class_data */
	  sizeof (EvaXmlFormat),
	  0,		/* n_preallocs */
	  (GInstanceInitFunc) NULL,
	  NULL		/* value_table */
	};
      type = g_type_register_static (G_TYPE_OBJECT,
				     "EvaXmlFormat",
				     &info,
				     0);
      g_type_add_interface_static (type,
				   EVA_TYPE_STORAGE_FORMAT,
				   &storage_format_info);
    }
  return type;
}
