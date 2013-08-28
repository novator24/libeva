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

/* EvaNetworkInterface:  find information about network devices on this host. */

#ifndef __EVA_NETWORK_INTERFACES_H_
#define __EVA_NETWORK_INTERFACES_H_

#include "evasocketaddress.h"

G_BEGIN_DECLS

typedef struct _EvaNetworkInterface EvaNetworkInterface;
typedef struct _EvaNetworkInterfaceSet EvaNetworkInterfaceSet;

typedef enum
{
  EVA_NETWORK_INTERFACE_UP			= (1<<0),
  EVA_NETWORK_INTERFACE_LOOPBACK		= (1<<1),
  EVA_NETWORK_INTERFACE_NON_LOOPBACK		= (1<<2),
  EVA_NETWORK_INTERFACE_HAS_BROADCAST		= (1<<3),
  EVA_NETWORK_INTERFACE_HAS_MULTICAST		= (1<<4)
} EvaNetworkInterfaceFlags;

struct _EvaNetworkInterface
{
  const char *ifname;

  /* whether this interface is "virtual" -- just connects back to this host */
  unsigned is_loopback : 1;

  /* whether this interface supports broadcasting. */
  unsigned supports_multicast : 1;

  /* whether this interface is receiving packets not intended for it. */
  unsigned is_promiscuous : 1;

  /* ip-address if the interface is up. */
  EvaSocketAddress *address;

  /* if !is_loopback, this is the device's MAC address. */
  EvaSocketAddress *hw_address;

  /* if is_point_to_point, this is the address of the other end of
   * the connection.
   */
  EvaSocketAddress *p2p_address;

  /* if supports_broadcast, this is the broadcast address. */
  EvaSocketAddress *broadcast;
};

struct _EvaNetworkInterfaceSet
{
  guint num_interfaces;
  EvaNetworkInterface *interfaces;
};

EvaNetworkInterfaceSet *
           eva_network_interface_set_new    (EvaNetworkInterfaceFlags  flags);
void       eva_network_interface_set_destroy(EvaNetworkInterfaceSet   *set);

G_END_DECLS

#endif
