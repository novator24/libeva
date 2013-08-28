/* "symbolic" addresses */

#ifndef __EVA_SOCKET_ADDRESS_SYMBOLIC_H_
#define __EVA_SOCKET_ADDRESS_SYMBOLIC_H_

#include "evasocketaddress.h"

typedef struct _EvaSocketAddressSymbolicClass EvaSocketAddressSymbolicClass;
typedef struct _EvaSocketAddressSymbolic EvaSocketAddressSymbolic;
typedef struct _EvaSocketAddressSymbolicIpv4Class EvaSocketAddressSymbolicIpv4Class;
typedef struct _EvaSocketAddressSymbolicIpv4 EvaSocketAddressSymbolicIpv4;

G_BEGIN_DECLS

GType eva_socket_address_symbolic_get_type(void) G_GNUC_CONST;
GType eva_socket_address_symbolic_ipv4_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_SOCKET_ADDRESS_SYMBOLIC              (eva_socket_address_symbolic_get_type ())
#define EVA_SOCKET_ADDRESS_SYMBOLIC(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_SOCKET_ADDRESS_SYMBOLIC, EvaSocketAddressSymbolic))
#define EVA_SOCKET_ADDRESS_SYMBOLIC_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_SOCKET_ADDRESS_SYMBOLIC, EvaSocketAddressSymbolicClass))
#define EVA_SOCKET_ADDRESS_SYMBOLIC_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_SOCKET_ADDRESS_SYMBOLIC, EvaSocketAddressSymbolicClass))
#define EVA_IS_SOCKET_ADDRESS_SYMBOLIC(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_SOCKET_ADDRESS_SYMBOLIC))
#define EVA_IS_SOCKET_ADDRESS_SYMBOLIC_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_SOCKET_ADDRESS_SYMBOLIC))
#define EVA_TYPE_SOCKET_ADDRESS_SYMBOLIC_IPV4              (eva_socket_address_symbolic_ipv4_get_type ())
#define EVA_SOCKET_ADDRESS_SYMBOLIC_IPV4(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_SOCKET_ADDRESS_SYMBOLIC_IPV4, EvaSocketAddressSymbolicIpv4))
#define EVA_SOCKET_ADDRESS_SYMBOLIC_IPV4_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_SOCKET_ADDRESS_SYMBOLIC_IPV4, EvaSocketAddressSymbolicIpv4Class))
#define EVA_SOCKET_ADDRESS_SYMBOLIC_IPV4_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_SOCKET_ADDRESS_SYMBOLIC_IPV4, EvaSocketAddressSymbolicIpv4Class))
#define EVA_IS_SOCKET_ADDRESS_SYMBOLIC_IPV4(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_SOCKET_ADDRESS_SYMBOLIC_IPV4))
#define EVA_IS_SOCKET_ADDRESS_SYMBOLIC_IPV4_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_SOCKET_ADDRESS_SYMBOLIC_IPV4))

typedef void (*EvaSocketAddressSymbolicResolveFunc) (EvaSocketAddressSymbolic *orig,
                                                     EvaSocketAddress         *resolved,
                                                     gpointer                  user_data);
typedef void (*EvaSocketAddressSymbolicErrorFunc)   (EvaSocketAddressSymbolic *orig,
                                                     const GError             *error,
                                                     gpointer                  user_data);

struct _EvaSocketAddressSymbolicClass
{
  EvaSocketAddressClass socket_address_class;
  gpointer (*create_name_resolver) (EvaSocketAddressSymbolic *symbolic);
  void     (*start_resolution)     (EvaSocketAddressSymbolic *,
                                    gpointer                  name_resolver,
                                    EvaSocketAddressSymbolicResolveFunc r,
                                    EvaSocketAddressSymbolicErrorFunc e,
                                    gpointer                  user_data,
                                    GDestroyNotify            destroy);
  void     (*cancel_resolution)    (EvaSocketAddressSymbolic *symbolic,
                                    gpointer                  name_resolver);

};
struct _EvaSocketAddressSymbolic
{
  EvaSocketAddress      socket_address;
  char                 *name;
};
struct _EvaSocketAddressSymbolicIpv4Class
{
  EvaSocketAddressSymbolicClass base_class;
};
struct _EvaSocketAddressSymbolicIpv4
{
  EvaSocketAddressSymbolic base_instance;
  guint16                  port;
};

EvaSocketAddress *eva_socket_address_symbolic_ipv4_new (const char *name,
                                                        guint16     port);


gpointer eva_socket_address_symbolic_create_name_resolver
                                     (EvaSocketAddressSymbolic *symbolic);
void     eva_socket_address_symbolic_start_resolution 
                                     (EvaSocketAddressSymbolic *symbolic,
                                      gpointer                  name_resolver,
                                      EvaSocketAddressSymbolicResolveFunc r,
                                      EvaSocketAddressSymbolicErrorFunc e,
                                      gpointer                  user_data,
                                      GDestroyNotify            destroy);
void     eva_socket_address_symbolic_cancel_resolution
                                     (EvaSocketAddressSymbolic *symbolic,
                                      gpointer                  name_resolver);

G_END_DECLS

#endif /* __EVA_SOCKET_ADDRESS_SYMBOLIC_H_ */
