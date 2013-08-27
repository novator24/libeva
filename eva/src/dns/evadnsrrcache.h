/*
    EVA - a library to write servers
    Copyright (C) 2001 Dave Benson

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

#ifndef __EVA_DNS_RR_CACHE_H_
#define __EVA_DNS_RR_CACHE_H_

#include "evadns.h"
#include <glib-object.h>
#include "../evasocketaddress.h"

G_BEGIN_DECLS

/* A cache of ResourceRecords.  For each host, we retain a list of ResourceRecords
 * with that host as `owner'.
 */

typedef struct _GskDnsRRCache GskDnsRRCache;

GType eva_dns_rr_cache_get_type () G_GNUC_CONST;
#define EVA_TYPE_DNS_RR_CACHE		(eva_dns_rr_cache_get_type ())

GskDnsRRCache        *eva_dns_rr_cache_new        (guint64                  max_bytes,
						   guint                    max_records);
GskDnsResourceRecord *eva_dns_rr_cache_insert     (GskDnsRRCache     *rr_cache,
					           const GskDnsResourceRecord    *record,
						   gboolean                 is_authoritative,
					           gulong                   cur_time);
void                  eva_dns_rr_cache_roundrobin (GskDnsRRCache           *rr_cache,
                                                   gboolean                 do_roundrobin);


/* Return a list of GskDnsResourceRecords.
 * You must lock those records if you plan on keeping them.
 * (Note: doesn't catch CNAMEs unless explicitly asked for.)
 */
GSList               *eva_dns_rr_cache_lookup_list(GskDnsRRCache           *rr_cache,
					           const char              *owner,
					           GskDnsResourceRecordType query_type,
					           GskDnsResourceClass      query_class);
typedef enum
{
  EVA_DNS_RR_CACHE_LOOKUP_DEREF_CNAMES = (1<<0)
} GskDnsRRCacheLookupFlags;
GskDnsResourceRecord *eva_dns_rr_cache_lookup_one (GskDnsRRCache           *rr_cache,
					           const char              *owner,
					           GskDnsResourceRecordType query_type,
					           GskDnsResourceClass      query_class,
						   GskDnsRRCacheLookupFlags flags);
gboolean              eva_dns_rr_cache_is_negative(GskDnsRRCache           *rr_cache,
					           const char              *owner,
					           GskDnsResourceRecordType query_type,
					           GskDnsResourceClass      query_class);

/* Prevent/allow a ResourceRecord from being freed from the cache
 * (the data itself may expire though!)
 */
void                  eva_dns_rr_cache_unlock     (GskDnsRRCache           *rr_cache,
	               			           GskDnsResourceRecord    *record);
void                  eva_dns_rr_cache_lock       (GskDnsRRCache           *rr_cache,
	               			           GskDnsResourceRecord    *record);
void                  eva_dns_rr_cache_mark_user  (GskDnsRRCache           *rr_cache,
			                           GskDnsResourceRecord    *record);
void                  eva_dns_rr_cache_unmark_user(GskDnsRRCache           *rr_cache,
			                           GskDnsResourceRecord    *record);

/* Negative caching.  RFC 1034, Section 4.3.4. */
/* A name error occurs if the error_code member of a GskDnsMessage
   is EVA_DNS_RESPONSE_ERROR_NAME_ERROR.  You must only cache the
   negative result during this query, unless a SOA record in the
   authority section exists for this name, in which case
   the 'minimum' field specifies a TTL for the negative result. */
void                  eva_dns_rr_cache_add_negative(GskDnsRRCache           *rr_cache,
						    const char              *owner,
					            GskDnsResourceRecordType query_type,
					            GskDnsResourceClass      query_class,
						    gulong                   expire_time,
						    gboolean                 is_authoritative);


/* master zone files */
gboolean              eva_dns_rr_cache_load_zone  (GskDnsRRCache           *rr_cache,
						   const char              *filename,
						   const char              *default_origin,
						   GError                 **error);

/* helper functions */

/* in the next two functions, caller must unref the *address_out if it returns TRUE  */
gboolean              eva_dns_rr_cache_get_ns_addr(GskDnsRRCache           *rr_cache,
						   const char              *host,
						   const char             **ns_name_out,
						   GskSocketAddressIpv4   **address_out);
gboolean              eva_dns_rr_cache_get_addr   (GskDnsRRCache           *rr_cache,
			                           const char              *host,
			                           GskSocketAddressIpv4   **address);

GskDnsRRCache *       eva_dns_rr_cache_ref        (GskDnsRRCache           *rr_cache);
void                  eva_dns_rr_cache_unref      (GskDnsRRCache           *rr_cache);

/* Flush out the oldest records in the cache. */
void                  eva_dns_rr_cache_flush      (GskDnsRRCache           *rr_cache,
	               			           gulong                   cur_time);

/* parsing an /etc/hosts file */
gboolean     eva_dns_rr_cache_parse_etc_hosts_line(GskDnsRRCache           *rr_cache,
				                   const char              *text);
gboolean     eva_dns_rr_cache_parse_etc_hosts     (GskDnsRRCache           *rr_cache,
				                   const char              *filename,
						   gboolean                 may_be_missing);


G_END_DECLS

#endif
