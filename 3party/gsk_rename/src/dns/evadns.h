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

#ifndef __EVA_DNS_H_
#define __EVA_DNS_H_

/* XXX: read about Inverse queries (1034, 3.7.2), Status queries (1034, 3.8).  */


/* XXX: decide:           is a EvaDnsZone structure a good idea?
 *                        cf 1034, 4.2.1, for a list of what's needed
 *                        to comprise a dns zone.
 */

/* XXX: EVA_DNS_RR_WELL_KNOWN_SERVICE */

/* EvaDnsMessage & friends: basic DNS protocol; client & server */


/* Well-known port for name-servers.  */
#define EVA_DNS_PORT		53

/* The DNS specification is divided between RFC 1034, Concepts & Facilities,
 * and RFC 1035, Implementation and Specification.
 */

typedef struct _EvaDnsMessage EvaDnsMessage;
typedef struct _EvaDnsResourceRecord EvaDnsResourceRecord;
typedef struct _EvaDnsQuestion EvaDnsQuestion;

#include "../evabuffer.h"
#include "../evapacket.h"
#include <stdio.h>			/* for FILE, ugh */

/* RFC 1034, 3.6:  Each node has a set of resource information,
 *                 which may be empty.
 *
 * A EvaDnsResourceRecord is one element of that set.
 *
 * All the terminology, and many comments, are from that section.
 */


/* EvaDnsResourceRecordType: AKA RTYPE: 
 *       Types of `RR's or `ResourceRecord's (values match RFC 1035, 3.2.2)
 */
typedef enum
{
  /* An `A' record:  the ip address of a host. */
  EVA_DNS_RR_HOST_ADDRESS = 1,

  /* A `NS' record:  the authoritative name server for the domain */
  EVA_DNS_RR_NAME_SERVER = 2,

  /* A `CNAME' record:  indicate another name for an alias. */
  EVA_DNS_RR_CANONICAL_NAME = 5,

  /* A `HINFO' record: identifies the CPU and OS used by a host */
  EVA_DNS_RR_HOST_INFO = 13,

  /* A `MX' record */
  EVA_DNS_RR_MAIL_EXCHANGE = 15,

  /* A `PTR' record:  a pointer to another part of the domain name space */
  EVA_DNS_RR_POINTER = 12,

  /* A `SOA' record:  identifies the start of a zone of authority [???] */
  EVA_DNS_RR_START_OF_AUTHORITY = 6,

  /* A `TXT' record:  miscellaneous text */
  EVA_DNS_RR_TEXT = 16,

  /* A `WKS' record:  description of a well-known service */
  EVA_DNS_RR_WELL_KNOWN_SERVICE = 11,

  /* A `AAAA' record: for IPv6 (see RFC 1886) */
  EVA_DNS_RR_HOST_ADDRESS_IPV6 = 28,

  /* --- only allowed for queries --- */

  /* A `AXFR' record: `special zone transfer QTYPE' */
  EVA_DNS_RR_ZONE_TRANSFER = 252,

  /* A `MAILB' record: matches all mail box related RRs (e.g. MB and MG). */
  EVA_DNS_RR_ZONE_MAILB = 253,

  /* A `*' record:  matches anything. */
  EVA_DNS_RR_WILDCARD = 255

} EvaDnsResourceRecordType;


/* EvaDnsResourceRecordClass: AKA RCLASS:
 *       Types of networks the RR can apply to (values from 1035, 3.2.4)
 */
typedef enum
{
  /* `IN': the Internet system */
  EVA_DNS_CLASS_INTERNET = 1,

  /* `CH': the Chaos system (rare) */
  EVA_DNS_CLASS_CHAOS = 3,

  /* `HS': Hesiod (rare) */
  EVA_DNS_CLASS_HESIOD = 4,

  /* --- only for queries --- */

  /* `*': any system */
  EVA_DNS_CLASS_WILDCARD = 255
} EvaDnsResourceClass;

struct _EvaDnsResourceRecord
{
  EvaDnsResourceRecordType  type;
  char                     *owner;     /* where the resource_record is found */
  guint32                   time_to_live;
  EvaDnsResourceClass       record_class;

  /* rdata: record type specific data */
  union
  {
    /* For EVA_DNS_RR_HOST_ADDRESS and EVA_DNS_CLASS_INTERNET */
    struct
    {
      guint8 ip_address[4];
    } a;

		/* unsupported */
    /* For EVA_DNS_RR_HOST_ADDRESS and EVA_DNS_CLASS_CHAOS */
    struct
    {
      char *chaos_name;
      guint16 chaos_address;
    } a_chaos;

    /* For EVA_DNS_RR_CNAME, EVA_DNS_RR_POINTER, EVA_DNS_RR_NAME_SERVER */
    char *domain_name;

    /* For EVA_DNS_RR_MAIL_EXCHANGE */
    struct
    {
      guint preference_value; /* "lower is better" */

      char *mail_exchange_host_name;
    } mx;

    /* For EVA_DNS_RR_TEXT */
    char *txt;

    /* For EVA_DNS_RR_HOST_INFO */
    struct
    {
      char *cpu;
      char *os;
    } hinfo;


    /* SOA: Start Of a zone of Authority.
     *
     * Comments cut-n-pasted from RFC 1035, 3.3.13.
     */
    struct
    {
      /* The domain-name of the name server that was the
	 original or primary source of data for this zone. */
      char *mname;

      /* specifies the mailbox of the
	 person responsible for this zone. */
      char *rname;

      /* The unsigned 32 bit version number of the original copy
	 of the zone.  Zone transfers preserve this value.  This
	 value wraps and should be compared using sequence space
	 arithmetic. */
      guint32 serial;

      /* A 32 bit time interval before the zone should be
	 refreshed. (cf 1034, 4.3.5) [in seconds] */
      guint32 refresh_time;

      /* A 32 bit time interval that should elapse before a
	 failed refresh should be retried. [in seconds] */
      guint32 retry_time;

      /* A 32 bit time value that specifies the upper limit on
	 the time interval that can elapse before the zone is no
	 longer authoritative. [in seconds] */
      guint32 expire_time;

      /* The unsigned 32 bit minimum TTL field that should be
	 exported with any RR from this zone. [in seconds] */
      guint32 minimum_time;
    } soa;

    struct {
      guint8 address[16];
    } aaaa;
  } rdata;

  /* private */
  EvaDnsMessage *allocator;
};

/* Test whether serial_1 is less that serial_2.  See RFC 1982.  */
#define EVA_DNS_SERIAL_LESS_THAN(serial_1, serial_2)		\
  (((gint32) ((guint32)(serial_1) - (guint32)(serial_2))) < 0)

/* --- constructing EvaDnsResourceRecords --- */
EvaDnsResourceRecord *eva_dns_rr_new_generic (EvaDnsMessage        *allocator,
					      const char           *owner,
					      guint32               ttl);
EvaDnsResourceRecord *eva_dns_rr_new_a       (const char           *owner,
					      guint32               ttl,
					      const guint8         *ip_address,
					      EvaDnsMessage        *allocator);
EvaDnsResourceRecord *eva_dns_rr_new_aaaa    (const char           *owner,
                                              guint32               ttl,
                                              const guint8         *address,
                                              EvaDnsMessage        *allocator);
EvaDnsResourceRecord *eva_dns_rr_new_ns      (const char           *owner,
					      guint32               ttl,
					      const char           *name_server,
					      EvaDnsMessage        *allocator);
EvaDnsResourceRecord *eva_dns_rr_new_cname   (const char           *owner,
					      guint32               ttl,
					      const char           *canonical_name,
					      EvaDnsMessage        *allocator);
EvaDnsResourceRecord *eva_dns_rr_new_ptr     (const char           *owner,
					      guint32               ttl,
					      const char           *ptr,
					      EvaDnsMessage        *allocator);
EvaDnsResourceRecord *eva_dns_rr_new_mx      (const char           *owner,
					      guint32               ttl,
					      int                   preference,
					      const char           *mail_host,
					      EvaDnsMessage        *allocator);
EvaDnsResourceRecord *eva_dns_rr_new_hinfo   (const char           *owner,
					      guint32               ttl,
					      const char           *cpu,
					      const char           *os,
					      EvaDnsMessage        *allocator);
EvaDnsResourceRecord *eva_dns_rr_new_soa     (const char           *owner,
					      guint32               ttl,
					      const char           *mname,
					      const char           *rname,
					      guint32               serial,
					      guint32               refresh_time,
					      guint32               retry_time,
					      guint32               expire_time,
					      guint32               minimum_time,
					      EvaDnsMessage        *allocator);
EvaDnsResourceRecord *eva_dns_rr_new_txt     (const char           *owner,
					      guint32               ttl,
					      const char           *text,
					      EvaDnsMessage        *allocator);
void                  eva_dns_rr_free        (EvaDnsResourceRecord *record);

EvaDnsResourceRecord *eva_dns_rr_copy        (EvaDnsResourceRecord *record,
					      EvaDnsMessage        *allocator);

/* queries only */

EvaDnsResourceRecord *eva_dns_rr_new_wildcard(const char    *owner,
					      guint          ttl,
					      EvaDnsMessage *allocator);

/* TODO: EVA_DNS_RR_ZONE_TRANSFER EVA_DNS_RR_ZONE_MAILB */

/* --- EvaDnsQuestion --- */
struct _EvaDnsQuestion
{
  /* The domain name for which information is being requested. */
  char                     *query_name;

  /* The type of query we are asking. */
  EvaDnsResourceRecordType  query_type;

  /* The domain where the query applies. */
  EvaDnsResourceClass       query_class;


  /*< private >*/
  EvaDnsMessage            *allocator;
};

EvaDnsQuestion *eva_dns_question_new (const char               *query_name,
				      EvaDnsResourceRecordType  query_type,
				      EvaDnsResourceClass       query_class,
				      EvaDnsMessage            *allocator);
EvaDnsQuestion *eva_dns_question_copy(EvaDnsQuestion           *question,
				      EvaDnsMessage            *allocator);
void            eva_dns_question_free(EvaDnsQuestion           *question);


/* --- Outputting and parsing text DNS resource records --- */
/* cf RFC 1034, Section 3.6.1. */

EvaDnsResourceRecord *eva_dns_rr_text_parse (const char           *record,
			                     const char           *last_owner,
					     const char           *origin,
			                     char                **err_msg,
					     EvaDnsMessage        *allocator);
char                 *eva_dns_rr_text_to_str(EvaDnsResourceRecord *rr,
					     const char           *last_owner);
char           *eva_dns_question_text_to_str(EvaDnsQuestion       *question);
void                  eva_dns_rr_text_write (EvaDnsResourceRecord *rr,
					     EvaBuffer            *out_buffer,
					     const char           *last_owner);


/* --- Response types --- */
/* Various error messages the server can send to a client. */
typedef enum
{
  EVA_DNS_RESPONSE_ERROR_NONE             =0,
  EVA_DNS_RESPONSE_ERROR_FORMAT           =1,
  EVA_DNS_RESPONSE_ERROR_SERVER_FAILURE   =2,
  EVA_DNS_RESPONSE_ERROR_NAME_ERROR       =3,
  EVA_DNS_RESPONSE_ERROR_NOT_IMPLEMENTED  =4,
  EVA_DNS_RESPONSE_ERROR_REFUSED          =5
} EvaDnsResponseCode;

/* DNS messages, divided into queries & answers.
 *
 * cf. RFC 1034, Section 3.7.
 */
struct _EvaDnsMessage
{
  /* Header: fixed data about all queries */
  guint16       id;       /* used by requestor to match queries and replies */

  /* Is this a query or a response? */
  guint16       is_query : 1;

  guint16       is_authoritative : 1;
  guint16       is_truncated : 1;

  /* [Responses only] the `RA bit': whether the server is willing to provide
   *                                recursive services. (cf 1034, 4.3.1)
   */
  guint16       recursion_available : 1;

  /* [Queries only] the `RD bit': whether the requester wants recursive
   *                              service for this queries. (cf 1034, 4.3.1)
   */
  guint16       recursion_desired : 1;

  /* Question: Carries the query name and other query parameters. */
  /* `qtype' (names are from RFC 1034, section 3.7.1): the query type */
  GSList       *questions;	/* of EvaDnsQuestion */

  /* Answer: Carries RRs which directly answer the query. */
  EvaDnsResponseCode error_code;
  GSList       *answers;	/* of EvaDnsResourceRecord */

  /* Authority: Carries RRs which describe other authoritative servers.
   *            May optionally carry the SOA RR for the authoritative
   *            data in the answer section.
   */
  GSList       *authority;	/* of EvaDnsResourceRecord */

  /* Additional: Carries RRs which may be helpful in using the RRs in the
   *             other sections.
   */
  GSList       *additional;	/* of EvaDnsResourceRecord */

  /*< private >*/
  guint         ref_count;
  GMemChunk    *qr_pool;  /* for EvaDnsQuestion and EvaDnsResourceRecord */
  GStringChunk *str_pool; /* for all strings */
  GHashTable   *offset_to_str;   /* for decompressing only */
};

/* --- binary dns messages --- */
EvaDnsMessage *eva_dns_message_new           (guint16        id,
					      gboolean       is_request);
EvaDnsMessage *eva_dns_message_parse_buffer  (EvaBuffer     *buffer);
EvaDnsMessage *eva_dns_message_parse_data    (const guint8  *data,
				              guint          length,
				              guint         *bytes_used_out);
void           eva_dns_message_write_buffer  (EvaDnsMessage *message,
				              EvaBuffer     *buffer,
				              gboolean       compress);
EvaPacket     *eva_dns_message_to_packet     (EvaDnsMessage *message);

/* --- adjusting DnsMessages --- */
/* (Just manipulate the lists directly, if there are a lot of changes to make.) */

/* the DnsMessage will free the added object */
void           eva_dns_message_append_question (EvaDnsMessage        *message,
						EvaDnsQuestion       *question);
void           eva_dns_message_append_answer   (EvaDnsMessage        *message,
						EvaDnsResourceRecord *answer);
void           eva_dns_message_append_auth     (EvaDnsMessage        *message,
						EvaDnsResourceRecord *auth);
void           eva_dns_message_append_addl     (EvaDnsMessage        *message,
						EvaDnsResourceRecord *addl);

/* calling these functions will free the second parameter */
void           eva_dns_message_remove_question (EvaDnsMessage        *message,
						EvaDnsQuestion       *question);
void           eva_dns_message_remove_answer   (EvaDnsMessage        *message,
						EvaDnsResourceRecord *answer);
void           eva_dns_message_remove_auth     (EvaDnsMessage        *message,
						EvaDnsResourceRecord *auth);
void           eva_dns_message_remove_addl     (EvaDnsMessage        *message,
						EvaDnsResourceRecord *addl);

/* refcounting */
void           eva_dns_message_unref           (EvaDnsMessage        *message);
void           eva_dns_message_ref             (EvaDnsMessage        *message);


/* for debugging: dump the message human-readably to a file pointer. */
void           eva_dns_dump_message_fp         (EvaDnsMessage        *message,
					        FILE                 *fp);
void           eva_dns_dump_question_fp        (EvaDnsQuestion       *question,
					        FILE                 *fp);

/*< private >*/
/* XXX: use eva_ipv4_parse() instead */
gboolean eva_dns_parse_ip_address (const char **pat,
		                   guint8      *ip_addr_out);

/* if you need to test whether a supplied name will be admitted
 * into the DNS domain. */
gboolean eva_test_domain_name_validity (const char *domain_name);

#endif
