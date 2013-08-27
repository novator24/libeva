#ifndef __EVA_STREAM_CLIENT_H_
#define __EVA_STREAM_CLIENT_H_

#include "evasocketaddress.h"
#include "evastreamfd.h"

G_BEGIN_DECLS

GskStream *eva_stream_new_connecting (GskSocketAddress  *address,
				      GError           **error);

/* see also: eva_socket_address_connect_fd
        and  eva_socket_address_finish_fd  (which has nothing to do with socket-addresses,
                                            but they're designed as a pair) */

G_END_DECLS

#endif
