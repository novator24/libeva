#include <string.h>
#include "evasocketaddresssymbolic.h"
#include "evanameresolver.h"

G_DEFINE_ABSTRACT_TYPE(EvaSocketAddressSymbolic,
                       eva_socket_address_symbolic,
		       EVA_TYPE_SOCKET_ADDRESS);
G_DEFINE_TYPE         (EvaSocketAddressSymbolicIpv4,
                       eva_socket_address_symbolic_ipv4,
		       EVA_TYPE_SOCKET_ADDRESS_SYMBOLIC);

/* EvaSocketAddressSymbolic */
enum
{
  SYMBOLIC_PROP_0,
  SYMBOLIC_PROP_NAME,
};
static gboolean
eva_socket_address_symbolic_to_native(EvaSocketAddress *address,
			              gpointer          output)
{
  return FALSE;
}

static gboolean
eva_socket_address_symbolic_from_native (EvaSocketAddress *address,
			                 gconstpointer     sockaddr_data,
			                 gsize             sockaddr_length)
{
  return FALSE;
}

static char *
eva_socket_address_symbolic_to_string  (EvaSocketAddress *address)
{
  return g_strdup (EVA_SOCKET_ADDRESS_SYMBOLIC (address)->name);
}

static gboolean
eva_socket_address_symbolic_equals     (EvaSocketAddress *addr1,
                                        EvaSocketAddress *addr2)
{
  return strcmp (EVA_SOCKET_ADDRESS_SYMBOLIC (addr1)->name,
                 EVA_SOCKET_ADDRESS_SYMBOLIC (addr2)->name) == 0;
}
static guint
eva_socket_address_symbolic_hash       (EvaSocketAddress *addr)
{
  return g_str_hash (EVA_SOCKET_ADDRESS_SYMBOLIC (addr)->name) + 100000;
}

static void
eva_socket_address_symbolic_set_property (GObject        *object,
                                          guint           property_id,
                                          const GValue   *value,
                                          GParamSpec     *pspec)
{
  EvaSocketAddressSymbolic *symbolic = EVA_SOCKET_ADDRESS_SYMBOLIC (object);
  switch (property_id)
    {
    case SYMBOLIC_PROP_NAME:
      g_free (symbolic->name);
      symbolic->name = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
eva_socket_address_symbolic_get_property (GObject        *object,
                                          guint           property_id,
                                          GValue         *value,
                                          GParamSpec     *pspec)
{
  EvaSocketAddressSymbolic *symbolic = EVA_SOCKET_ADDRESS_SYMBOLIC (object);
  switch (property_id)
    {
    case SYMBOLIC_PROP_NAME:
      g_value_set_string (value, symbolic->name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}
static void
eva_socket_address_symbolic_finalize (GObject *object)
{
  EvaSocketAddressSymbolic *symbolic = EVA_SOCKET_ADDRESS_SYMBOLIC (object);
  g_free (symbolic->name);
  G_OBJECT_CLASS (eva_socket_address_symbolic_parent_class)->finalize (object);
}

static void
eva_socket_address_symbolic_class_init (EvaSocketAddressSymbolicClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  EvaSocketAddressClass *address_class = EVA_SOCKET_ADDRESS_CLASS (class);
  address_class->to_native = eva_socket_address_symbolic_to_native;
  address_class->from_native = eva_socket_address_symbolic_from_native;
  address_class->to_string = eva_socket_address_symbolic_to_string;
  address_class->hash = eva_socket_address_symbolic_hash;
  address_class->equals = eva_socket_address_symbolic_equals;
  object_class->set_property = eva_socket_address_symbolic_set_property;
  object_class->get_property = eva_socket_address_symbolic_get_property;
  object_class->finalize = eva_socket_address_symbolic_finalize;
  g_object_class_install_property (object_class, SYMBOLIC_PROP_NAME,
                                   g_param_spec_string ("name", "Name", "name (of host)",
                                                        NULL, G_PARAM_READWRITE));
}
static void
eva_socket_address_symbolic_init (EvaSocketAddressSymbolic *symbolic)
{
}

/* Public API */
gpointer
eva_socket_address_symbolic_create_name_resolver (EvaSocketAddressSymbolic *symbolic)
{
  EvaSocketAddressSymbolicClass *class = EVA_SOCKET_ADDRESS_SYMBOLIC_GET_CLASS (symbolic);
  return class->create_name_resolver (symbolic);
}

void
eva_socket_address_symbolic_start_resolution (EvaSocketAddressSymbolic *symbolic,
                                              gpointer                  name_resolver,
                                              EvaSocketAddressSymbolicResolveFunc r,
                                              EvaSocketAddressSymbolicErrorFunc e,
                                              gpointer                  user_data,
                                              GDestroyNotify            destroy)
{
  EvaSocketAddressSymbolicClass *class = EVA_SOCKET_ADDRESS_SYMBOLIC_GET_CLASS (symbolic);
  class->start_resolution (symbolic, name_resolver, r, e, user_data, destroy);
}
void
eva_socket_address_symbolic_cancel_resolution (EvaSocketAddressSymbolic *symbolic,
                                               gpointer                  name_resolver)
{
  EvaSocketAddressSymbolicClass *class = EVA_SOCKET_ADDRESS_SYMBOLIC_GET_CLASS (symbolic);
  class->cancel_resolution (symbolic, name_resolver);
}

/* EvaSocketAddressSymbolicIpv4 */
enum
{
  SYMBOLIC_IPV4_PROP_0,
  SYMBOLIC_IPV4_PROP_PORT,
};

typedef struct _Ipv4NameResolver Ipv4NameResolver;
struct _Ipv4NameResolver
{
  EvaSocketAddressSymbolicIpv4 *ipv4;
  EvaSocketAddressSymbolicResolveFunc resolve_func;
  EvaSocketAddressSymbolicErrorFunc error_func;
  gpointer data;
  GDestroyNotify destroy;
  EvaNameResolverTask *task;
  gboolean resolved;
};

static gpointer 
eva_socket_address_symbolic_ipv4_create_name_resolver (EvaSocketAddressSymbolic *symbolic)
{
  Ipv4NameResolver *resolver = g_new0 (Ipv4NameResolver, 1);
  resolver->ipv4 = EVA_SOCKET_ADDRESS_SYMBOLIC_IPV4 (g_object_ref (symbolic));
  return resolver;
}

static void
ipv4_handle_success (EvaSocketAddress *address,
                     gpointer          func_data)
{
  Ipv4NameResolver *resolver = func_data;

  EvaSocketAddressIpv4 *host_only = EVA_SOCKET_ADDRESS_IPV4 (address);
  EvaSocketAddress *real;

  resolver->resolved = 1;

  /* prepare new socket address */
  real = eva_socket_address_ipv4_new (host_only->ip_address,
                                      resolver->ipv4->port);

  /* invoke user's callback */
  (*resolver->resolve_func) (EVA_SOCKET_ADDRESS_SYMBOLIC (resolver->ipv4),
                             real,
                             resolver->data);

  /* cleanup */
  g_object_unref (real);
}

static void
ipv4_handle_failure (GError           *error,
                     gpointer          func_data)
{
  Ipv4NameResolver *resolver = func_data;
  resolver->resolved = 1;

  /* invoke user's callback */
  if (resolver->error_func != NULL)
    (*resolver->error_func) (EVA_SOCKET_ADDRESS_SYMBOLIC (resolver->ipv4),
                             error,
                             resolver->data);
}

static void
ipv4_handle_destroy (gpointer          func_data)
{
  Ipv4NameResolver *resolver = func_data;
  g_assert (resolver->resolved);
  if (resolver->destroy != NULL)
    resolver->destroy (resolver->data);
  g_object_unref (resolver->ipv4);
  g_free (resolver);
}

static void
eva_socket_address_symbolic_ipv4_start_resolution (EvaSocketAddressSymbolic *symbolic,
                                                   gpointer                  name_resolver,
                                                   EvaSocketAddressSymbolicResolveFunc r,
                                                   EvaSocketAddressSymbolicErrorFunc e,
                                                   gpointer                  user_data,
                                                   GDestroyNotify            destroy)
{
  Ipv4NameResolver *resolver = name_resolver;
  resolver->resolve_func = r;
  resolver->error_func = e;
  resolver->data = user_data;
  resolver->destroy = destroy;
  resolver->resolved = 0;
  resolver->task = eva_name_resolver_task_new (EVA_NAME_RESOLVER_FAMILY_IPV4,
                                               symbolic->name,
                                               ipv4_handle_success,
                                               ipv4_handle_failure,
                                               resolver,
                                               ipv4_handle_destroy);

  /* note: may destroy resolver! */
  eva_name_resolver_task_unref (resolver->task);
}

static void
eva_socket_address_symbolic_ipv4_cancel_resolution (EvaSocketAddressSymbolic *symbolic,
                                                    gpointer                  name_resolver)
{
  Ipv4NameResolver *resolver = name_resolver;

  /* if this triggers, i guess that we are in
     the start_resolution() callback,
     and name resolution hasn't blocked,
     so that either "success" or "failure" 
     callback is being invoked.  but then,
     how did the user get the pointer "resolver"?
     it's impossible. */
  g_return_if_fail (resolver->task != NULL);

  if (resolver->resolved)
    return;
  if (resolver->task != NULL)
    {
      EvaNameResolverTask *task = resolver->task;
      resolver->task = NULL;
      eva_name_resolver_task_cancel (task);
    }
}


static char *
eva_socket_address_symbolic_ipv4_to_string  (EvaSocketAddress *address)
{
  return g_strdup_printf ("%s:%u",
                          EVA_SOCKET_ADDRESS_SYMBOLIC (address)->name,
                          (guint) EVA_SOCKET_ADDRESS_SYMBOLIC_IPV4 (address)->port);
}

static gboolean
eva_socket_address_symbolic_ipv4_equals(EvaSocketAddress *addr1,
                                        EvaSocketAddress *addr2)
{
  return strcmp (EVA_SOCKET_ADDRESS_SYMBOLIC (addr1)->name,
                 EVA_SOCKET_ADDRESS_SYMBOLIC (addr2)->name) == 0
     &&   EVA_SOCKET_ADDRESS_SYMBOLIC_IPV4 (addr1)->port
       == EVA_SOCKET_ADDRESS_SYMBOLIC_IPV4 (addr2)->port;
}
static guint
eva_socket_address_symbolic_ipv4_hash   (EvaSocketAddress *addr)
{
  return g_str_hash (EVA_SOCKET_ADDRESS_SYMBOLIC (addr)->name) + 200000
     + EVA_SOCKET_ADDRESS_SYMBOLIC_IPV4 (addr)->port;
}
static void
eva_socket_address_symbolic_ipv4_set_property (GObject        *object,
                                               guint           property_id,
                                               const GValue   *value,
                                               GParamSpec     *pspec)
{
  EvaSocketAddressSymbolicIpv4 *symbolic_ipv4 = EVA_SOCKET_ADDRESS_SYMBOLIC_IPV4 (object);
  switch (property_id)
    {
    case SYMBOLIC_IPV4_PROP_PORT:
      symbolic_ipv4->port = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
eva_socket_address_symbolic_ipv4_get_property (GObject        *object,
                                               guint           property_id,
                                               GValue         *value,
                                               GParamSpec     *pspec)
{
  EvaSocketAddressSymbolicIpv4 *symbolic_ipv4 = EVA_SOCKET_ADDRESS_SYMBOLIC_IPV4 (object);
  switch (property_id)
    {
    case SYMBOLIC_IPV4_PROP_PORT:
      g_value_set_uint (value, symbolic_ipv4->port);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
eva_socket_address_symbolic_ipv4_finalize (GObject *object)
{
  //EvaSocketAddressSymbolicIpv4 *symbolic_ipv4 = EVA_SOCKET_ADDRESS_SYMBOLIC_IPV4 (object);
  G_OBJECT_CLASS (eva_socket_address_symbolic_ipv4_parent_class)->finalize (object);
}

static void
eva_socket_address_symbolic_ipv4_class_init (EvaSocketAddressSymbolicIpv4Class *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  EvaSocketAddressClass *address_class = EVA_SOCKET_ADDRESS_CLASS (class);
  EvaSocketAddressSymbolicClass *symbolic_class = EVA_SOCKET_ADDRESS_SYMBOLIC_CLASS (class);
  object_class->set_property = eva_socket_address_symbolic_ipv4_set_property;
  object_class->get_property = eva_socket_address_symbolic_ipv4_get_property;
  object_class->finalize = eva_socket_address_symbolic_ipv4_finalize;
  address_class->to_string = eva_socket_address_symbolic_ipv4_to_string;
  address_class->hash = eva_socket_address_symbolic_ipv4_hash;
  address_class->equals = eva_socket_address_symbolic_ipv4_equals;
  symbolic_class->create_name_resolver = eva_socket_address_symbolic_ipv4_create_name_resolver;
  symbolic_class->start_resolution = eva_socket_address_symbolic_ipv4_start_resolution;
  symbolic_class->cancel_resolution = eva_socket_address_symbolic_ipv4_cancel_resolution;
  g_object_class_install_property (object_class, SYMBOLIC_IPV4_PROP_PORT,
                                   g_param_spec_uint ("port", "Port", "TCP port",
                                                      0,65535,0, G_PARAM_READWRITE));
}
static void
eva_socket_address_symbolic_ipv4_init (EvaSocketAddressSymbolicIpv4 *symbolic_ipv4)
{
}
EvaSocketAddress *
eva_socket_address_symbolic_ipv4_new (const char *name,
                                      guint16     port)
{
  return g_object_new (EVA_TYPE_SOCKET_ADDRESS_SYMBOLIC_IPV4,
                       "name", name,
                       "port", port,
                       NULL);
}
