#include "evadnsserver.h"
#include "../evaghelpers.h"
#include "../evamacros.h"

enum
{
  PROP_0,
  PROP_PACKET_QUEUE,
  PROP_RESOLVER
};

struct _EvaDnsServerClass 
{
  GObjectClass		base_class;
};
struct _EvaDnsServer 
{
  GObject		base_instance;

  EvaPacketQueue       *packet_queue;

  EvaDnsResolver       *resolver;
  guint                 recursion_available : 1;
  guint                 is_blocking_write : 1;

  GSList               *first_outgoing_packet;
  GSList               *last_outgoing_packet;

  GSList               *tasks;
};

/* --- prototypes --- */
static GObjectClass *parent_class = NULL;


typedef struct _ServerTask ServerTask;
struct _ServerTask
{
  /* bookkeeping data */
  EvaDnsResolverTask *task;
  EvaDnsServer *server;

  /* whether we are used the recursing cache */
  gboolean doing_recursive_lookup;

  /* the message and where it came from */
  EvaSocketAddress *return_address;
  EvaDnsMessage *question;
};

static void
eva_dns_server_transmit_packet (ServerTask   *task,
				EvaPacket    *packet)
{
  EvaDnsServer *server = task->server;
  if (server->first_outgoing_packet == NULL)
    {
      GError *error = NULL;
      if (!eva_packet_queue_write (server->packet_queue, packet, &error))
	{
	  if (error)
	    {
	      g_warning ("unable to transmit packet to server: %s", error->message);
	      g_error_free (error);
	      return;
	    }
	}
      else
	{
	  return;
	}
    }
  server->last_outgoing_packet = g_slist_append (server->last_outgoing_packet, packet);
  if (server->first_outgoing_packet)
    server->last_outgoing_packet = server->last_outgoing_packet->next;
  else
    server->first_outgoing_packet = server->last_outgoing_packet;
  eva_packet_ref (packet);
  if (server->is_blocking_write)
    {
      server->is_blocking_write = 0;
      eva_io_unblock_write (EVA_IO (server->packet_queue));
    }
}

static GSList *
duplicate_rr_list (GSList *rr_list, EvaDnsMessage *message)
{
  GSList *rv = NULL;
  while (rr_list != NULL)
    {
      EvaDnsResourceRecord *rr = rr_list->data;
      rr_list = rr_list->next;
      rv = g_slist_prepend (rv, eva_dns_rr_copy (rr, message));
    }
  return g_slist_reverse (rv);
}

static void 
server_task_resolve_result(GSList             *answers,
			   GSList             *authority,
			   GSList             *additional,
			   GSList             *negatives,
			   gpointer            func_data)
{
  ServerTask *server_task = func_data;
  EvaDnsMessage *response;
  EvaPacket *packet;
  response = eva_dns_message_new (server_task->question->id, FALSE);
  response->recursion_desired = server_task->question->recursion_desired;
  response->recursion_available = server_task->question->recursion_desired
			       && server_task->server->recursion_available;
  response->answers = duplicate_rr_list (answers, response);
  response->authority = duplicate_rr_list (authority, response);
  response->additional = duplicate_rr_list (additional, response);
  if (negatives != NULL)
    response->error_code = EVA_DNS_RESPONSE_ERROR_NAME_ERROR;
  packet = eva_dns_message_to_packet (response);
  eva_dns_message_unref (response);
  eva_packet_set_dst_address (packet, server_task->return_address);

  eva_dns_server_transmit_packet (server_task, packet);
  eva_packet_unref (packet);
}

static void
server_task_on_fail (GError             *error,
		     gpointer            func_data)
{
  ServerTask *server_task = func_data;
  EvaDnsMessage *response;
  EvaPacket *packet;
  response = eva_dns_message_new (server_task->question->id, FALSE);

  if (error->domain != EVA_G_ERROR_DOMAIN)
    {
      response->error_code = EVA_DNS_RESPONSE_ERROR_SERVER_FAILURE;
    }
  else
    {
      /* Create a failure response. */
      /* XXX: it'd be great to return the data that we do know!!! */
      switch (error->code)
	{
	case EVA_ERROR_RESOLVER_TOO_MANY_FAILURES:
	case EVA_ERROR_RESOLVER_NO_NAME_SERVERS:
	  response->error_code = EVA_DNS_RESPONSE_ERROR_SERVER_FAILURE;
	  break;

	default:
	  g_warning ("server_task_on_fail: error=%s", error->message);
	  response->error_code = EVA_DNS_RESPONSE_ERROR_SERVER_FAILURE;
	  return;
	}
    }
  packet = eva_dns_message_to_packet (response);
  eva_dns_message_unref (response);
  eva_packet_set_dst_address (packet, server_task->return_address);
  eva_dns_server_transmit_packet (server_task, packet);
  eva_packet_unref (packet);
}

static void
server_task_destroy (gpointer            func_data)
{
  ServerTask *server_task = func_data;
  if (server_task->task != NULL)
    server_task->server->tasks = g_slist_remove (server_task->server->tasks,
						 server_task->task);
  eva_dns_message_unref (server_task->question);
  g_free (server_task);
}

static void
server_handle_incoming_messages(EvaDnsMessage    *message,
				EvaSocketAddress *socket_address,
				gpointer          user_data)
{
  EvaDnsServer *server = EVA_DNS_SERVER (user_data);
  ServerTask *server_task;
  EvaDnsResolverTask *task;
  EvaDnsResolver *resolver;
  EvaDnsResolverHints hints;

  if (!message->is_query)
    return;

  resolver = server->resolver;
  g_return_if_fail (resolver != NULL);

  server_task = g_new (ServerTask, 1);
  server_task->return_address = g_object_ref (socket_address);
  server_task->question = message;
  server_task->task = NULL;
  server_task->server = server;

  eva_dns_message_ref (message);

  hints.address = socket_address;

  task = eva_dns_resolver_resolve (resolver,
				   message->recursion_desired,
				   message->questions,
				   server_task_resolve_result,
				   server_task_on_fail,
				   server_task,
				   server_task_destroy,
				   &hints);
  if (task != NULL)
    {
      server_task->task = task;
      server->tasks = g_slist_prepend (server->tasks, task);
    }
}

static void
cancel_all_server_tasks (EvaDnsServer *server)
{
  while (server->tasks != NULL)
    eva_dns_resolver_cancel (server->resolver,
			     (EvaDnsResolverTask *) (server->tasks->data));

}

/* --- i/o handlers --- */
static gboolean
eva_dns_server_handle_readable (EvaIO        *io,
				gpointer      data)
{
  EvaDnsServer *server = EVA_DNS_SERVER (data);
  GError *error = NULL;
  EvaPacket *packet = eva_packet_queue_read (server->packet_queue, TRUE, &error);
  EvaDnsMessage *message;
  guint used;
  if (!packet)
    {
      if (error)
	{
	  g_warning ("error reading packet: %s", error->message);
	  g_error_free (error);
	  return FALSE;
	}
      /* false-alarm */
      return TRUE;
    }

  message = eva_dns_message_parse_data (packet->data, packet->len, &used);
  if (!message)
    {
      g_warning ("error parsing dns message");
      eva_packet_unref (packet);
      return FALSE;
    }
  server_handle_incoming_messages (message, packet->src_address, server);
  eva_packet_unref (packet);
  return TRUE;
}

static gboolean
eva_dns_server_handle_readable_shutdown (EvaIO        *io,
				         gpointer      data)
{
  EvaDnsServer *server = EVA_DNS_SERVER (data);
  g_return_val_if_fail (EVA_IS_DNS_SERVER (server), FALSE);
  return FALSE;
}

static gboolean
eva_dns_server_handle_writable (EvaIO        *io,
				gpointer      data)
{
  EvaDnsServer *server = EVA_DNS_SERVER (data);
  EvaPacket *packet;
  GError *error = NULL;
  if (server->first_outgoing_packet == NULL)
    {
      g_assert (!server->is_blocking_write);
      eva_io_block_write (io);
      server->is_blocking_write = 1;
      return TRUE;
    }

  packet = server->first_outgoing_packet->data;
  if (!eva_packet_queue_write (server->packet_queue, packet, &error))
    {
      if (error)
	{
	  g_warning ("unable to write packet (dns-server): %s", error->message);
	  cancel_all_server_tasks (server);
	  return FALSE;
	}

      /* false alarm */
      return TRUE;
    }

  server->first_outgoing_packet = g_slist_remove (server->first_outgoing_packet, packet);
  if (server->first_outgoing_packet == NULL)
    server->last_outgoing_packet = NULL;
  eva_packet_unref (packet);
  return TRUE;
}

static gboolean
eva_dns_server_handle_writable_shutdown (EvaIO        *io,
				         gpointer      data)
{
  EvaDnsServer *server = EVA_DNS_SERVER (data);
  cancel_all_server_tasks (server);
  return FALSE;
}

/* --- GObject methods --- */
static GObject *
eva_dns_server_constructor (GType                  type,
		            guint                  n_c_properties,
		            GObjectConstructParam *c_properties)
{
  GObject *rv = (*parent_class->constructor) (type, n_c_properties, c_properties);
  EvaDnsServer *server = EVA_DNS_SERVER (rv);
  EvaIO *io;
  if (server->packet_queue == NULL)
    {
      g_object_unref (rv);
      return NULL;
    }
  io = EVA_IO (server->packet_queue);
  eva_io_trap_readable (io,
			eva_dns_server_handle_readable,
			eva_dns_server_handle_readable_shutdown,
			server,
			NULL);
  eva_io_trap_writable (io,
			eva_dns_server_handle_writable,
			eva_dns_server_handle_writable_shutdown,
			server,
			NULL);
  return rv;
}


static void
eva_dns_server_get_property (GObject        *object,
			     guint           property_id,
			     GValue         *value,
			     GParamSpec     *pspec)
{
  EvaDnsServer *server = EVA_DNS_SERVER (object);
  switch (property_id)
    {
    case PROP_PACKET_QUEUE:
      {
	EvaPacketQueue *queue = server->packet_queue;
	if (queue)
	  g_value_set_object (value, g_object_ref (queue));
	else
	  g_value_set_object (value, NULL);
        break;
      }
    case PROP_RESOLVER:
      {
	EvaDnsResolver *resolver = server->resolver;
	if (resolver)
	  g_value_set_object (value, g_object_ref (resolver));
	else
	  g_value_set_object (value, NULL);
        break;
      }
    }
}

static void
eva_dns_server_set_property (GObject        *object,
			     guint           property_id,
			     const GValue   *value,
			     GParamSpec     *pspec)
{
  EvaDnsServer *server = EVA_DNS_SERVER (object);
  switch (property_id)
    {
    case PROP_PACKET_QUEUE:
      {
	EvaPacketQueue *queue = server->packet_queue;
	EvaPacketQueue *new_queue = g_value_get_object (value);
	if (new_queue)
	  g_object_ref (new_queue);
	if (queue)
	  g_object_unref (queue);
	server->packet_queue = new_queue;
        break;
      }
    case PROP_RESOLVER:
      {
	EvaDnsResolver *resolver = server->resolver;
	EvaDnsResolver *new_resolver = g_value_get_object (value);
	if (new_resolver)
	  g_object_ref (new_resolver);
	if (resolver)
	  g_object_unref (resolver);
	server->resolver = new_resolver;
        break;
      }
    }
}


static void
eva_dns_server_finalize (GObject *object)
{
  cancel_all_server_tasks (EVA_DNS_SERVER (object));
  (*parent_class->finalize) (object);
}

/* --- class functions --- */
static void
eva_dns_server_init (EvaDnsServer *dns_server)
{
  (void) dns_server;
}

static void
eva_dns_server_class_init (EvaDnsServerClass *dns_server_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (dns_server_class);
  GParamSpec *pspec;
  parent_class = g_type_class_peek_parent (dns_server_class);
  object_class->finalize = eva_dns_server_finalize;
  object_class->set_property = eva_dns_server_set_property;
  object_class->get_property = eva_dns_server_get_property;
  object_class->constructor = eva_dns_server_constructor;

  pspec = g_param_spec_object ("packet-queue",
			       _("Packet Queue"),
			       _("raw i/o handle to use for requests/responses"),
			       EVA_TYPE_PACKET_QUEUE,
			       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_PACKET_QUEUE, pspec);

  pspec = g_param_spec_object ("resolver",
			       _("Resolver"),
			       _("DNS resolver to obtain responses from"),
			       EVA_TYPE_DNS_RESOLVER,
			       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_RESOLVER, pspec);
}

GType
eva_dns_server_get_type()
{
  static GType dns_server_type = 0;
  if (!dns_server_type)
    {
      static const GTypeInfo dns_server_info =
      {
	sizeof(EvaDnsServerClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_dns_server_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EvaDnsServer),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_dns_server_init,
	NULL		/* value_table */
      };
      dns_server_type = g_type_register_static (G_TYPE_OBJECT,
                                                  "EvaDnsServer",
						  &dns_server_info, 0);
    }
  return dns_server_type;
}

/**
 * eva_dns_server_new:
 * @resolver: the resolver to query for answers to incoming questions.
 * @packet_queue: transport for incoming DNS requests and outgoing responses.
 *
 * Allocate a DNS server using the optional @resolver to answer questions.
 *
 * returns: the newly allocated DNS server.
 */
EvaDnsServer *
eva_dns_server_new           (EvaDnsResolver     *resolver,
			      EvaPacketQueue     *queue)
{
  EvaDnsServer *server;
  g_return_val_if_fail (queue != NULL, NULL);
  if (resolver)
    server = g_object_new (EVA_TYPE_DNS_SERVER,
			   "packet-queue", queue,
			   "resolver", resolver,
			   NULL);
  else
    server = g_object_new (EVA_TYPE_DNS_SERVER,
			   "packet-queue", queue, NULL);
  return EVA_DNS_SERVER (server);
}

/**
 * eva_dns_server_peek_resolver:
 * @server: the server to inspect.
 *
 * Obtain a peeked reference at the resolver which this server is using to
 * answer questions.
 *
 * returns: a EvaDnsResolver if one is being used, or NULL.
 */
EvaDnsResolver *
eva_dns_server_peek_resolver (EvaDnsServer       *server)
{
  return server->resolver;
}

/**
 * eva_dns_server_set_resolver:
 * @server: the server to affect.
 * @resolver: the DNS resolver to use, or NULL to stop using any resolver.
 *
 * Set the DNS server's resolver.
 */
void
eva_dns_server_set_resolver (EvaDnsServer       *server,
			     EvaDnsResolver     *resolver)
{
  if (resolver != NULL)
    g_object_ref (resolver);
  if (server->resolver != NULL)
    g_object_unref (server->resolver);
  server->resolver = resolver;
}
