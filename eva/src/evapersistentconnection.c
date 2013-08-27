#include "evapersistentconnection.h"
#include "evastreamclient.h"
#include "evanameresolver.h"

G_DEFINE_TYPE(GskPersistentConnection, eva_persistent_connection, EVA_TYPE_STREAM);

static guint handle_connected_signal_id = 0;
static guint handle_disconnected_signal_id = 0;

static void eva_persistent_connection_set_poll_read (GskIO    *io,
                                                     gboolean  should_poll);
static void eva_persistent_connection_set_poll_write(GskIO    *io,
                                                     gboolean  should_poll);

static inline void
maybe_message (GskPersistentConnection *connection,
               const char              *verb)
{
  if (connection->debug_connection)
    {
      char *location = eva_socket_address_to_string (connection->address);
      g_message ("%s %s", verb, location);
      g_free (location);
    }
}

static void
eva_persistent_connection_handle_disconnected (GskPersistentConnection *connection)
{
  maybe_message (connection, "disconnected from");
}

static void
eva_persistent_connection_handle_connected (GskPersistentConnection *connection)
{
  maybe_message (connection, "connected to");
}

static guint
eva_persistent_connection_raw_read(GskStream     *stream,
			 	   gpointer       data,
			 	   guint          length,
			 	   GError       **error)
{
  GskPersistentConnection *connection = EVA_PERSISTENT_CONNECTION (stream);
  if (connection->transport == NULL)
    return 0;
  return eva_stream_read (connection->transport, data, length, error);
}

static guint
eva_persistent_connection_raw_write(GskStream     *stream,
			 	    gconstpointer  data,
			 	    guint          length,
			 	    GError       **error)
{
  GskPersistentConnection *connection = EVA_PERSISTENT_CONNECTION (stream);
  if (connection->transport == NULL)
    return 0;
  return eva_stream_write (connection->transport, data, length, error);
}

static guint
eva_persistent_connection_raw_read_buffer(GskStream     *stream,
				          GskBuffer     *buffer,
				          GError       **error)
{
  GskPersistentConnection *connection = EVA_PERSISTENT_CONNECTION (stream);
  if (connection->transport == NULL)
    return 0;
  return eva_stream_read_buffer (connection->transport, buffer, error);
}

static guint
eva_persistent_connection_raw_write_buffer(GskStream    *stream,
                                           GskBuffer     *buffer,
				           GError       **error)
{
  GskPersistentConnection *connection = EVA_PERSISTENT_CONNECTION (stream);
  if (connection->transport == NULL)
    return 0;
  return eva_stream_write_buffer (connection->transport, buffer, error);
}

static void
shutdown_transport (GskPersistentConnection *connection)
{
  if (eva_io_has_write_hook (connection->transport))
    eva_io_untrap_writable (connection->transport);
  if (eva_io_has_read_hook (connection->transport))
    eva_io_untrap_readable (connection->transport);
  eva_io_shutdown (EVA_IO (connection->transport), NULL);
  if (connection->state == EVA_PERSISTENT_CONNECTION_CONNECTING)
    g_signal_handler_disconnect (G_OBJECT (connection->transport),
                         connection->transport_on_connect_signal_handler);
  g_signal_handler_disconnect (G_OBJECT (connection->transport),
                       connection->transport_on_error_signal_handler);
  g_object_unref (connection->transport);
  connection->transport = NULL;
}

static void
eva_persistent_connection_finalize (GObject *object)
{
  GskPersistentConnection *connection = EVA_PERSISTENT_CONNECTION (object);
  if (connection->transport != NULL)
    shutdown_transport (connection);
  if (connection->retry_timeout_source)
    {
      GskSource *source = connection->retry_timeout_source;
      connection->retry_timeout_source = NULL;
      eva_source_remove (source);
    }
  G_OBJECT_CLASS (eva_persistent_connection_parent_class)->finalize (object);
}

static void
eva_persistent_connection_init (GskPersistentConnection *connection)
{
  eva_io_mark_is_readable (connection);
  eva_io_mark_is_writable (connection);
}

static void
eva_persistent_connection_class_init (GskPersistentConnectionClass *class)
{
  GskIOClass *io_class = EVA_IO_CLASS (class);
  GskStreamClass *stream_class = EVA_STREAM_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  class->handle_connected = eva_persistent_connection_handle_connected;
  class->handle_disconnected = eva_persistent_connection_handle_disconnected;
  io_class->set_poll_read = eva_persistent_connection_set_poll_read;
  io_class->set_poll_write = eva_persistent_connection_set_poll_write;
  stream_class->raw_read = eva_persistent_connection_raw_read;
  stream_class->raw_write = eva_persistent_connection_raw_write;
  stream_class->raw_read_buffer = eva_persistent_connection_raw_read_buffer;
  stream_class->raw_write_buffer = eva_persistent_connection_raw_write_buffer;
  object_class->finalize = eva_persistent_connection_finalize;


  handle_connected_signal_id
    = g_signal_new ("handle-connected",
                    G_OBJECT_CLASS_TYPE (object_class),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (GskPersistentConnectionClass, handle_connected),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE,
                    0);
  handle_disconnected_signal_id
    = g_signal_new ("handle-disconnected",
                    G_OBJECT_CLASS_TYPE (object_class),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (GskPersistentConnectionClass, handle_disconnected),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE,
                    0);
}

static gboolean handle_retry_timeout_expired (gpointer data);

static void
setup_timeout (GskPersistentConnection *connection)
{
  g_return_if_fail (connection->retry_timeout_source == NULL);
  connection->retry_timeout_source
    = eva_main_loop_add_timer (eva_main_loop_default (),
                               handle_retry_timeout_expired,
                               connection,
                               NULL,
                               connection->retry_timeout_ms,
                               -1);
  connection->state = EVA_PERSISTENT_CONNECTION_WAITING;
}

static void
handle_transport_connected (GskStream *stream,
                            GskPersistentConnection *connection)
{
  g_return_if_fail (connection->transport == stream);
  g_return_if_fail (connection->state == EVA_PERSISTENT_CONNECTION_CONNECTING);
  connection->state = EVA_PERSISTENT_CONNECTION_CONNECTED;
  g_signal_handler_disconnect (stream,
                               connection->transport_on_connect_signal_handler);
  g_signal_emit (connection, handle_connected_signal_id, 0);
}

static gboolean
handle_transport_readable (GskStream               *transport,
                           GskPersistentConnection *connection)
{
  g_return_val_if_fail (connection->transport == transport, FALSE);
  eva_io_notify_ready_to_read (connection);
  return TRUE;
}

static gboolean
handle_transport_read_shutdown (GskStream           *transport,
                                GskPersistentConnection *connection)
{
  g_return_val_if_fail (connection->transport == transport, FALSE);
  g_assert (connection->state == EVA_PERSISTENT_CONNECTION_CONNECTED
        ||  connection->state == EVA_PERSISTENT_CONNECTION_CONNECTING);
  if (eva_io_has_write_hook (transport))
    eva_io_untrap_writable (transport);
  shutdown_transport (connection);
  connection->state = EVA_PERSISTENT_CONNECTION_WAITING;
  g_signal_emit (connection, handle_disconnected_signal_id, 0);
  setup_timeout (connection);
  return FALSE;
}

static gboolean
handle_transport_writable (GskStream           *transport,
                           GskPersistentConnection *connection)
{
  g_return_val_if_fail (connection->transport == transport, FALSE);
  eva_io_notify_ready_to_write (connection);
  return TRUE;
}

static void
handle_transport_error (GskStream *transport,
                        GskPersistentConnection *connection)
{
  g_return_if_fail (connection->transport == transport);
  if (connection->warn_on_transport_errors)
    g_warning ("error in transport: %s", EVA_IO (transport)->error->message);
  shutdown_transport (connection);
  g_signal_emit (connection, handle_disconnected_signal_id, 0);
  setup_timeout (connection);
}

static gboolean
handle_transport_write_shutdown (GskStream           *transport,
                                 GskPersistentConnection *connection)
{
  g_return_val_if_fail (connection->transport == transport, FALSE);
  g_assert (connection->state == EVA_PERSISTENT_CONNECTION_CONNECTED
         || connection->state == EVA_PERSISTENT_CONNECTION_CONNECTING);

  if (eva_io_has_read_hook (transport))
    eva_io_untrap_readable (transport);
  shutdown_transport (connection);
  connection->state = EVA_PERSISTENT_CONNECTION_WAITING;
  g_signal_emit (connection, handle_disconnected_signal_id, 0);
  setup_timeout (connection);

  return FALSE;
}

static void
retry_connection (GskPersistentConnection *connection,
                  GskSocketAddress        *address)
{
  GError *error = NULL;
  GskStream *transport = eva_stream_new_connecting (address, &error);
  if (transport == NULL)
    {
      eva_io_set_gerror (EVA_IO (connection),
                         EVA_IO_ERROR_CONNECT,
                         error);
      setup_timeout (connection);
      return;
    }
  connection->transport = transport;
  if (EVA_STREAM_FD (transport)->is_resolving_name
   || eva_io_get_is_connecting (transport))
    {
      connection->state = EVA_PERSISTENT_CONNECTION_CONNECTING;
      connection->transport_on_connect_signal_handler
        = g_signal_connect (transport,
                            "on-connect",
                            G_CALLBACK (handle_transport_connected),
                            connection);
    }
  else
    {
      connection->state = EVA_PERSISTENT_CONNECTION_CONNECTED;
      g_signal_emit (connection, handle_connected_signal_id, 0);
    }
  if (eva_io_is_polling_for_read (connection))
    eva_io_trap_readable (transport,
                          handle_transport_readable,
                          handle_transport_read_shutdown,
                          connection,
                          NULL);
  if (eva_io_is_polling_for_write (connection))
    eva_io_trap_writable (transport,
                          handle_transport_writable,
                          handle_transport_write_shutdown,
                          connection,
                          NULL);
  connection->transport_on_error_signal_handler
    = g_signal_connect (transport,
                        "on-error",
                        G_CALLBACK (handle_transport_error),
                        connection);
}

static void eva_persistent_connection_set_poll_read (GskIO    *io,
                                                     gboolean  should_poll)
{
  GskPersistentConnection *connection = EVA_PERSISTENT_CONNECTION (io);
  if (connection->transport)
    {
      if (should_poll)
        eva_io_trap_readable (EVA_IO (connection->transport),
                              handle_transport_readable,
                              handle_transport_read_shutdown,
                              connection,
                              NULL);
      else
        eva_io_untrap_readable (EVA_IO (connection->transport));
    }
}

static void eva_persistent_connection_set_poll_write (GskIO    *io,
                                                      gboolean  should_poll)
{
  GskPersistentConnection *connection = EVA_PERSISTENT_CONNECTION (io);
  if (connection->transport)
    {
      if (should_poll)
        eva_io_trap_writable (EVA_IO (connection->transport),
                              handle_transport_writable,
                              handle_transport_write_shutdown,
                              connection,
                              NULL);
      else
        eva_io_untrap_writable (EVA_IO (connection->transport));
    }
}

static gboolean
handle_retry_timeout_expired (gpointer data)
{
  GskPersistentConnection *connection = EVA_PERSISTENT_CONNECTION (data);
  connection->retry_timeout_source = NULL;
  if (connection->address != NULL)
    retry_connection (connection, connection->address);
  else
    g_warning ("no address???");
  return FALSE;
}

GskStream *
eva_persistent_connection_new (GskSocketAddress *address,
                               guint             retry_timeout_ms)
{
  GskPersistentConnection *connection = g_object_new (EVA_TYPE_PERSISTENT_CONNECTION, NULL);
  connection->address = g_object_ref (address);
  connection->retry_timeout_ms = retry_timeout_ms;
  retry_connection (connection, address);
  return EVA_STREAM (connection);
}

GskStream *
eva_persistent_connection_new_lookup (const char *host,
                                      guint       port,
                                      guint       retry_timeout_ms)
{
  GskSocketAddress *symbolic = eva_socket_address_symbolic_ipv4_new (host, port);
  GskStream *pc = eva_persistent_connection_new (symbolic, retry_timeout_ms);
  g_object_unref (symbolic);
  return pc;
}

void eva_persistent_connection_restart (GskPersistentConnection *connection,
                                        guint                    retry_wait_ms)
{
  if (connection->transport != NULL)
    {
      shutdown_transport (connection);
      g_signal_emit (connection, handle_disconnected_signal_id, 0);
    }
  if (connection->retry_timeout_source != NULL)
    {
      eva_source_remove (connection->retry_timeout_source);
      connection->retry_timeout_source = NULL;
    }
  connection->retry_timeout_source
    = eva_main_loop_add_timer (eva_main_loop_default (),
                               handle_retry_timeout_expired,
                               connection,
                               NULL,
                               retry_wait_ms,
                               -1);
  connection->state = EVA_PERSISTENT_CONNECTION_WAITING;
}
