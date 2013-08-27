#define G_IMPLEMENT_INLINES
#define __EVA_STORE_C__
#include <string.h>
#include "evastreammap.h"
#include "../evastreamtransferrequest.h"
#include "evastore.h"

static EvaStoreFormatEntry *
get_format_entry_for_type (EvaStore *store, GType value_type, GError **error)
{
  GPtrArray *format_entries = store->format_entries;
  guint i;

  (void) error;
  g_return_val_if_fail (format_entries, NULL);
  for (;;)
    {
      for (i = 0; i < format_entries->len; ++i)
	{
	  EvaStoreFormatEntry *entry = g_ptr_array_index (format_entries, i);
	  if (entry->value_type == value_type)
	    return entry;
	}
      if (value_type == G_TYPE_INVALID)
	break;
      value_type = g_type_parent (value_type);
    }
  return NULL;
}

static EvaStoreFormatEntry *
get_format_entry_for_id (EvaStore *store, guint32 format_id, GError **error)
{
  GPtrArray *format_entries = store->format_entries;
  guint i;

  (void) error;
  g_return_val_if_fail (format_entries, NULL);
  for (i = 0; i < format_entries->len; ++i)
    {
      EvaStoreFormatEntry *entry = g_ptr_array_index (format_entries, i);
      if (entry->format_id == format_id)
	return entry;
    }
  return NULL;
}

/*
 *
 * Save
 *
 */

typedef struct _SaveInfo SaveInfo;

struct _SaveInfo
{
  EvaStreamTransferRequest *xfer_request;
};

/* Signal handler invoked when the EvaStreamTransferRequest is done
 * transferring the stream of serialized data to the write-stream
 * from the EvaStreamMap.
 */
static void
save_handle_xfer_request_done (EvaRequest *xfer_request, gpointer user_data)
{
  EvaStoreRequest *store_request = EVA_STORE_REQUEST (user_data);
  SaveInfo *save_info = (SaveInfo *) store_request->private;

  g_return_if_fail (store_request->request_type == EVA_STORE_REQUEST_SAVE);
  g_return_if_fail (save_info->xfer_request ==
		    EVA_STREAM_TRANSFER_REQUEST (xfer_request));

  if (eva_request_had_error (xfer_request))
    {
      GError *error;
      error = g_error_copy (eva_request_get_error (xfer_request));
      eva_request_set_error (store_request, error);
    }
  g_object_unref (xfer_request);
  save_info->xfer_request = NULL;

  eva_request_done (store_request);
  g_object_unref (store_request);
}

static void
save_start (EvaStoreRequest *store_request)
{
  SaveInfo *save_info = (SaveInfo *) store_request->private;
  EvaStreamTransferRequest *xfer_request;

  g_return_if_fail (store_request->request_type == EVA_STORE_REQUEST_SAVE);
  xfer_request = save_info->xfer_request;
  g_return_if_fail (xfer_request);

  g_object_ref (store_request);
  g_signal_connect (xfer_request,
		    "done",
		    G_CALLBACK (save_handle_xfer_request_done),
		    store_request);
  eva_request_start (xfer_request);
}

EvaStoreRequest *
eva_store_save (EvaStore      *store,
		const char    *key,
		const GValue  *value,
		GError       **error)
{
  EvaStreamMap *stream_map = store->stream_map;
  EvaStoreFormatEntry *format_entry;
  EvaStream *serialize_stream;
  EvaStream *write_stream;
  EvaStreamTransferRequest *xfer_request;
  EvaStoreRequest *store_request;
  SaveInfo *save_info;

  format_entry = get_format_entry_for_type (store, G_VALUE_TYPE (value), error);
  if (format_entry == NULL)
    {
      g_return_val_if_fail (error == NULL || *error, NULL);
      return NULL;
    }
  g_return_val_if_fail (format_entry->storage_format, NULL);
  serialize_stream = eva_storage_format_serialize (format_entry->storage_format,
						   value,
						   error);
  if (serialize_stream == NULL)
    {
      g_return_val_if_fail (error == NULL || *error, NULL);
      return NULL;
    }
  write_stream = eva_stream_map_set (stream_map, key, error);
  if (write_stream == NULL)
    {
      g_object_unref (serialize_stream);
      g_return_val_if_fail (error == NULL || *error, NULL);
      return NULL;
    }
  xfer_request =
    eva_stream_transfer_request_new (serialize_stream, write_stream);
  g_return_val_if_fail (xfer_request, NULL);
  g_object_unref (serialize_stream);
  g_object_unref (write_stream);

/* HACK: maybe EvaStreamTransferRequest should support this. */
  {
    guint32 format_id_out = format_entry->format_id;

#if G_BYTE_ORDER != G_LITTLE_ENDIAN
    format_id_out = GUINT32_TO_LE (format_id_out);
#endif
    eva_buffer_append (&xfer_request->buffer,
		       &format_id_out,
		       sizeof (format_id_out));
  }

  save_info = g_new0 (SaveInfo, 1);
  save_info->xfer_request = xfer_request;

  store_request = g_object_new (EVA_TYPE_STORE_REQUEST, NULL);
  store_request->request_type = EVA_STORE_REQUEST_SAVE;
  store_request->key = g_strdup (key);
  store_request->private = save_info;
  return store_request;
}

/*
 *
 * Load
 *
 */

typedef struct _LoadInfo LoadInfo;

struct _LoadInfo
{
  GType value_type;
  EvaStream *read_stream;
  EvaStore *store;
};

static void
load_handle_deserialize_request_done (EvaRequest *request, gpointer user_data)
{
  EvaStoreRequest *store_request = EVA_STORE_REQUEST (user_data);
  EvaValueRequest *deserialize_request = EVA_VALUE_REQUEST (request);
  GError *error = NULL;
  const GValue *value;

  if (eva_request_had_error (deserialize_request))
    {
      error = g_error_copy (eva_request_get_error (deserialize_request));
      eva_request_set_error (store_request, error);
      eva_request_done (store_request);
      g_object_unref (store_request);
      return;
    }

  value = eva_value_request_get_value (deserialize_request);
  if (value == NULL)
    {
      error = g_error_new (EVA_G_ERROR_DOMAIN,
			   EVA_ERROR_UNKNOWN, /* TODO: code */
			   "couldn't get value from a %s",
			   g_type_name (G_OBJECT_TYPE (deserialize_request)));
      eva_request_set_error (store_request, error);
      eva_request_done (store_request);
      g_object_unref (store_request);
      return;
    }

  g_value_init (&store_request->value_request.value, G_VALUE_TYPE (value));
  g_value_copy (value, &store_request->value_request.value);
  eva_request_done (store_request);
  g_object_unref (store_request);
}

static gboolean
load_handle_input_is_readable (EvaIO *io, gpointer user_data)
{
  EvaStoreRequest *store_request = EVA_STORE_REQUEST (user_data);
  LoadInfo *load_info = (LoadInfo *) store_request->private;
  EvaStream *read_stream;
  guint32 format_id;
  EvaStoreFormatEntry *format_entry;
  EvaValueRequest *deserialize_request;
  GError *error = NULL;
  guint num_read;

  g_return_val_if_fail (store_request->request_type == EVA_STORE_REQUEST_LOAD,
			FALSE);
  g_return_val_if_fail (load_info, FALSE);
  read_stream = load_info->read_stream;
  g_return_val_if_fail (read_stream == EVA_STREAM (io), FALSE);

  /* XXX: We assume we can read 4 bytes in one try. Probably safe for
   * any reasonable stream, but...
   */
  num_read = eva_stream_read (read_stream,
			      &format_id,
			      sizeof (format_id),
			      &error);
  if (error)
    {
      eva_request_set_error (store_request, error);
      eva_request_done (store_request);
      return FALSE;
    }
  g_return_val_if_fail (num_read == sizeof (format_id), FALSE);
#if G_BYTE_ORDER != G_LITTLE_ENDIAN
  format_id = GUINT32_FROM_LE (format_id);
#endif

  eva_io_untrap_readable (read_stream);

  g_return_val_if_fail (load_info->store, FALSE);
  format_entry = get_format_entry_for_id (load_info->store, format_id, &error);
  if (format_entry == NULL)
    {
      eva_request_set_error (store_request, error);
      eva_request_done (store_request);
      return FALSE;
    }

  deserialize_request =
    eva_storage_format_deserialize (format_entry->storage_format,
				    read_stream,
				    load_info->value_type,
				    &error);
  if (deserialize_request == NULL)
    {
      eva_request_set_error (store_request, error);
      eva_request_done (store_request);
      g_return_val_if_fail (error, FALSE);
      return FALSE;
    }

  g_object_ref (store_request);
  g_signal_connect (deserialize_request,
		    "done",
		    G_CALLBACK (load_handle_deserialize_request_done),
		    store_request);
  eva_request_start (deserialize_request);

  /* Note: we've already untrapped the hook above, so the return value
   * is meaningless.
   */
  return FALSE;
}

static gboolean
load_handle_input_shutdown_read (EvaIO *io, gpointer user_data)
{
  EvaStoreRequest *store_request = EVA_STORE_REQUEST (user_data);
  GError *error;

  (void) io;
  g_return_val_if_fail (store_request->request_type == EVA_STORE_REQUEST_LOAD,
			FALSE);

  error = g_error_new (EVA_G_ERROR_DOMAIN,
		       EVA_ERROR_UNKNOWN, /* TODO: code */
		       "premature end of stream (%s)",
		       g_type_name (G_OBJECT_TYPE (io)));
  eva_request_set_error (store_request, error);
  eva_request_done (store_request);
  return FALSE;
}

static void
load_handle_input_is_readable_destroy (gpointer user_data)
{
  EvaStoreRequest *store_request = EVA_STORE_REQUEST (user_data);
  LoadInfo *load_info = (LoadInfo *) store_request->private;

  g_return_if_fail (store_request->request_type == EVA_STORE_REQUEST_LOAD);
  g_return_if_fail (load_info);

  g_object_unref (load_info->read_stream);
  load_info->read_stream = NULL;
  g_object_unref (store_request);
}

static void
load_start (EvaStoreRequest *store_request)
{
  LoadInfo *load_info = (LoadInfo *) store_request->private;
  EvaStream *read_stream;

  g_return_if_fail (store_request->request_type == EVA_STORE_REQUEST_LOAD);
  g_return_if_fail (load_info);
  read_stream = load_info->read_stream;
  g_return_if_fail (read_stream);
  g_return_if_fail (eva_stream_get_is_readable (read_stream));
  g_return_if_fail (!eva_io_has_read_hook (read_stream));

  g_object_ref (store_request);
  eva_io_trap_readable (EVA_IO (read_stream),
			load_handle_input_is_readable,
			load_handle_input_shutdown_read,
			store_request,
			load_handle_input_is_readable_destroy);
}

EvaStoreRequest *
eva_store_load (EvaStore    *store,
		const char  *key,
		GType        value_type,
		GError     **error)
{
  EvaStreamMap *stream_map = store->stream_map;
  EvaStream *read_stream;
  EvaStoreRequest *store_request;
  LoadInfo *load_info;

  read_stream = eva_stream_map_get (stream_map, key, error);
  if (read_stream == NULL)
    {
      g_return_val_if_fail (error == NULL || *error, NULL);
      return NULL;
    }

  load_info = g_new0 (LoadInfo, 1);
  load_info->store = store;
  g_object_ref (store);
  load_info->value_type = value_type;
  load_info->read_stream = read_stream;

  store_request = g_object_new (EVA_TYPE_STORE_REQUEST, NULL);
  store_request->request_type = EVA_STORE_REQUEST_LOAD;
  store_request->key = g_strdup (key);
  store_request->private = load_info;
  return store_request;
}

/*
 *
 * Delete
 *
 */

typedef struct _DeleteInfo DeleteInfo;

struct _DeleteInfo
{
  EvaStreamMapRequest *delete_request;
};

/* Signal handler invoked when the EvaStreamMapRequest is done. */
static void
delete_handle_request_done (EvaRequest *delete_request, gpointer user_data)
{
  EvaStoreRequest *store_request = EVA_STORE_REQUEST (user_data);
  DeleteInfo *delete_info = (DeleteInfo *) store_request->private;

  g_return_if_fail (store_request->request_type == EVA_STORE_REQUEST_DELETE);
  g_return_if_fail (delete_info);
  g_return_if_fail (delete_info->delete_request ==
		    EVA_STREAM_MAP_REQUEST (delete_request));

  g_object_unref (store_request);
  if (eva_request_had_error (delete_request))
    {
      GError *error;
      error = g_error_copy (eva_request_get_error (delete_request));
      eva_request_set_error (store_request, error);
    }
  g_object_unref (delete_request);
  delete_info->delete_request = NULL;

  eva_request_done (store_request);
}

static void
delete_start (EvaStoreRequest *store_request)
{
  DeleteInfo *delete_info = (DeleteInfo *) store_request->private;

  g_return_if_fail (store_request->request_type == EVA_STORE_REQUEST_DELETE);
  g_return_if_fail (delete_info);

  g_object_ref (store_request);
  g_signal_connect (delete_info->delete_request,
		    "done",
		    G_CALLBACK (delete_handle_request_done),
		    store_request);
  eva_request_start (delete_info->delete_request);
}

EvaStoreRequest *
eva_store_delete (EvaStore    *store,
		  const char  *key,
		  GError     **error)
{
  EvaStreamMap *stream_map = store->stream_map;
  EvaStreamMapRequest *delete_request;
  DeleteInfo *delete_info;
  EvaStoreRequest *store_request;

  delete_request = eva_stream_map_delete (stream_map, key, error);
  if (delete_request == NULL)
    {
      g_return_val_if_fail (error == NULL || *error, NULL);
      return NULL;
    }

  delete_info = g_new0 (DeleteInfo, 1);
  delete_info->delete_request = delete_request;

  store_request = g_object_new (EVA_TYPE_STORE_REQUEST, NULL);
  store_request->request_type = EVA_STORE_REQUEST_DELETE;
  store_request->key = g_strdup (key);
  store_request->private = delete_info;
  return store_request;
}

/*
 *
 * Exists
 *
 */

typedef struct _ExistsInfo ExistsInfo;

struct _ExistsInfo
{
  EvaStreamMapRequest *exists_request;
};

/* Signal handler invoked when the EvaStreamMapRequest is done. */
static void
exists_handle_request_done (EvaRequest *exists_request, gpointer user_data)
{
  EvaStoreRequest *store_request = EVA_STORE_REQUEST (user_data);
  ExistsInfo *exists_info = (ExistsInfo *) store_request->private;

  g_return_if_fail (store_request->request_type == EVA_STORE_REQUEST_EXISTS);
  g_return_if_fail (exists_info);
  g_return_if_fail (exists_info->exists_request ==
		    EVA_STREAM_MAP_REQUEST (exists_request));

  g_object_unref (store_request);
  if (eva_request_had_error (exists_request))
    {
      GError *error;
      error = g_error_copy (eva_request_get_error (exists_request));
      eva_request_set_error (store_request, error);
    }
  g_value_init (&store_request->value_request.value, G_TYPE_BOOLEAN);
  g_value_set_boolean (&store_request->value_request.value,
		       eva_stream_map_request_get_exists (exists_request));
  g_object_unref (exists_request);
  exists_info->exists_request = NULL;

  eva_request_done (store_request);
}

static void
exists_start (EvaStoreRequest *store_request)
{
  ExistsInfo *exists_info = (ExistsInfo *) store_request->private;

  g_return_if_fail (store_request->request_type == EVA_STORE_REQUEST_EXISTS);
  g_return_if_fail (exists_info);

  g_object_ref (store_request);
  g_signal_connect (exists_info->exists_request,
		    "done",
		    G_CALLBACK (exists_handle_request_done),
		    store_request);
  eva_request_start (exists_info->exists_request);
}

EvaStoreRequest *
eva_store_exists (EvaStore    *store,
		  const char  *key,
		  GError     **error)
{
  EvaStreamMap *stream_map = store->stream_map;
  EvaStreamMapRequest *exists_request;
  ExistsInfo *exists_info;
  EvaStoreRequest *store_request;

  exists_request = eva_stream_map_exists (stream_map, key, error);
  if (exists_request == NULL)
    {
      g_return_val_if_fail (error == NULL || *error, NULL);
      return NULL;
    }

  exists_info = g_new0 (ExistsInfo, 1);
  exists_info->exists_request = exists_request;

  store_request = g_object_new (EVA_TYPE_STORE_REQUEST, NULL);
  store_request->request_type = EVA_STORE_REQUEST_EXISTS;
  store_request->key = g_strdup (key);
  store_request->private = exists_info;
  return store_request;
}

GType
eva_store_get_type (void)
{
  static GType type = 0;
  if (type == 0)
    {
      static const GTypeInfo info =
	{
	  sizeof (EvaStoreClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) NULL,
	  NULL,		/* class_finalize */
	  NULL,		/* class_data */
	  sizeof (EvaStore),
	  0,		/* n_preallocs */
	  (GInstanceInitFunc) NULL,
	  NULL		/* value_table */
	};
      type = g_type_register_static (G_TYPE_OBJECT,
				     "EvaStore",
				     &info,
				     0);
    }
  return type;
}

/*
 *
 * EvaStoreRequest
 *
 */

static GObjectClass *eva_store_request_parent_class = NULL;

/* EvaRequest methods. */
/* TODO: cancelled methods */

static void
eva_store_request_cancelled (EvaRequest *request)
{
  EvaStoreRequest *store_request = EVA_STORE_REQUEST (request);
  switch (store_request->request_type)
    {
      case EVA_STORE_REQUEST_LOAD:
//	load_cancel (store_request);
	break;
      case EVA_STORE_REQUEST_SAVE:
//	save_cancel (store_request);
	break;
      case EVA_STORE_REQUEST_DELETE:
//	delete_cancel (store_request);
	break;
      case EVA_STORE_REQUEST_EXISTS:
//	exists_cancel (store_request);
	break;
      default:
        g_return_if_reached ();
        break;
    }
}

static void
eva_store_request_start (EvaRequest *request)
{
  EvaStoreRequest *store_request = EVA_STORE_REQUEST (request);
  switch (store_request->request_type)
    {
      case EVA_STORE_REQUEST_LOAD:
	load_start (store_request);
	break;
      case EVA_STORE_REQUEST_SAVE:
	save_start (store_request);
	break;
      case EVA_STORE_REQUEST_DELETE:
	delete_start (store_request);
	break;
      case EVA_STORE_REQUEST_EXISTS:
	exists_start (store_request);
	break;
      default:
        g_return_if_reached ();
        break;
    }
}

/* GObject methods. */

static void
eva_store_request_finalize (GObject *object)
{
  EvaStoreRequest *store_request = EVA_STORE_REQUEST (object);

  if (store_request->key)
    g_free (store_request->key);

  switch (store_request->request_type)
    {
      case EVA_STORE_REQUEST_LOAD:
	/* TODO */
	break;
      case EVA_STORE_REQUEST_SAVE:
	/* TODO */
	break;
      case EVA_STORE_REQUEST_DELETE:
	/* TODO */
	break;
      case EVA_STORE_REQUEST_EXISTS:
	/* TODO */
	break;
      default:
	g_return_if_reached ();
        break;
    }
  (*eva_store_request_parent_class->finalize) (object);
}

static void
eva_store_request_class_init (EvaRequestClass *request_class)
{
  eva_store_request_parent_class = g_type_class_peek_parent (request_class);

  G_OBJECT_CLASS (request_class)->finalize = eva_store_request_finalize;
  request_class->start = eva_store_request_start;
  request_class->cancelled = eva_store_request_cancelled;
}

GType
eva_store_request_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0))
    {
      static const GTypeInfo type_info =
	{
	  sizeof (EvaStoreRequestClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) eva_store_request_class_init,
	  NULL,		/* class_finalize */
	  NULL,		/* class_data */
	  sizeof (EvaStoreRequest),
	  0,		/* n_preallocs */
	  (GInstanceInitFunc) NULL,
	  NULL		/* value_table */
	};
      type = g_type_register_static (EVA_TYPE_VALUE_REQUEST,
				     "EvaStoreRequest",
				     &type_info,
				     0);
    }
  return type;
}

/*
 *
 * EvaStoreFormatEntry
 *
 */

static GObjectClass *eva_store_format_entry_parent_class = NULL;

/* GObject methods. */

static void
eva_store_format_entry_finalize (GObject *object)
{
  EvaStoreFormatEntry *store_format_entry = EVA_STORE_FORMAT_ENTRY (object);

  if (store_format_entry->storage_format)
    g_object_unref (store_format_entry->storage_format);

  (*eva_store_format_entry_parent_class->finalize) (object);
}

static void
eva_store_format_entry_class_init (GObjectClass *object_class)
{
  eva_store_format_entry_parent_class = g_type_class_peek_parent (object_class);
  object_class->finalize = eva_store_format_entry_finalize;
}

GType
eva_store_format_entry_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0))
    {
      static const GTypeInfo type_info =
	{
	  sizeof (EvaStoreFormatEntryClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) eva_store_format_entry_class_init,
	  NULL,		/* class_finalize */
	  NULL,		/* class_data */
	  sizeof (EvaStoreFormatEntry),
	  0,		/* n_preallocs */
	  (GInstanceInitFunc) NULL,
	  NULL		/* value_table */
	};
      type = g_type_register_static (G_TYPE_OBJECT,
				     "EvaStoreFormatEntry",
				     &type_info,
				     0);
    }
  return type;
}
