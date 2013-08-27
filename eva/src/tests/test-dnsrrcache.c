#include "../evadns.h"
#include <time.h>
#include <string.h>

int main (int argc, char **argv)
{
  GskDnsRRCache *rr_cache;
  GskDnsMessage *allocator;
  GskDnsResourceRecord *rr, *found;
  GskDnsResourceRecord *copy;
  GskSocketAddressIpv4 *addr;
  gulong cur_time = time(NULL);
  char *err_msg = NULL;
  guint8 one_two_three_four[4] = {1,2,3,4};
  eva_init_without_threads (&argc, &argv);

  rr_cache = eva_dns_rr_cache_new (1024 * 1024, 1024);

  allocator = eva_dns_message_new (0, FALSE);
  rr = eva_dns_rr_new_a ("foo.bar", 1000, one_two_three_four, allocator);
  eva_dns_rr_cache_insert (rr_cache,rr,TRUE,cur_time);
  found = eva_dns_rr_cache_lookup_one (rr_cache, "foo.bar", EVA_DNS_RR_HOST_ADDRESS, EVA_DNS_CLASS_INTERNET,
                                       EVA_DNS_RR_CACHE_LOOKUP_DEREF_CNAMES);
  g_assert (found != NULL);
  g_assert (found->type == EVA_DNS_RR_HOST_ADDRESS);
  g_assert (found->record_class == EVA_DNS_CLASS_INTERNET);
  g_assert (memcmp (found->rdata.a.ip_address, one_two_three_four, 4) == 0);
  copy = eva_dns_rr_copy (found, NULL);
  g_assert (copy != NULL);
  g_assert (copy->type == EVA_DNS_RR_HOST_ADDRESS);
  g_assert (copy->record_class == EVA_DNS_CLASS_INTERNET);
  g_assert (memcmp (copy->rdata.a.ip_address, one_two_three_four, 4) == 0);
  eva_dns_rr_free (copy);

  g_assert (eva_dns_rr_cache_get_addr (rr_cache, "foo.bar", &addr));
  g_assert (addr != NULL);
  g_assert (memcmp (addr->ip_address, one_two_three_four, 4) == 0);
  g_object_unref (addr);
  addr = NULL;
  eva_dns_message_unref (allocator);

  allocator = eva_dns_message_new (0, FALSE);
  rr = eva_dns_rr_new_cname ("foo.baz", 1000, "foo.bar", allocator);
  eva_dns_rr_cache_insert (rr_cache,rr,TRUE,cur_time);
  found = eva_dns_rr_cache_lookup_one (rr_cache, "foo.baz", EVA_DNS_RR_CANONICAL_NAME, EVA_DNS_CLASS_INTERNET,
                                       EVA_DNS_RR_CACHE_LOOKUP_DEREF_CNAMES);
  g_assert (found != NULL);
  g_assert (found->type == EVA_DNS_RR_CANONICAL_NAME);
  g_assert (found->record_class == EVA_DNS_CLASS_INTERNET);
  g_assert (strcmp (found->rdata.domain_name, "foo.bar") == 0);
  copy = eva_dns_rr_copy (found, NULL);
  g_assert (copy != NULL);
  g_assert (copy->type == EVA_DNS_RR_CANONICAL_NAME);
  g_assert (copy->record_class == EVA_DNS_CLASS_INTERNET);
  g_assert (strcmp (copy->rdata.domain_name, "foo.bar") == 0);
  eva_dns_rr_free (copy);
  found = eva_dns_rr_cache_lookup_one (rr_cache, "foo.baz", EVA_DNS_RR_HOST_ADDRESS, EVA_DNS_CLASS_INTERNET,
                                       EVA_DNS_RR_CACHE_LOOKUP_DEREF_CNAMES);
  g_assert (found != NULL);
  g_assert (found->type == EVA_DNS_RR_HOST_ADDRESS);
  g_assert (found->record_class == EVA_DNS_CLASS_INTERNET);
  g_assert (memcmp (found->rdata.a.ip_address, one_two_three_four, 4) == 0);
  g_assert (eva_dns_rr_cache_get_addr (rr_cache, "foo.baz", &addr));
  g_assert (addr != NULL);
  g_assert (memcmp (addr->ip_address, one_two_three_four, 4) == 0);
  g_object_unref (addr);
  addr = NULL;
  eva_dns_message_unref (allocator);

  allocator = eva_dns_message_new (0, FALSE);
  rr = eva_dns_rr_new_hinfo ("foo.bar", 1000, "pentium", "linux", allocator);
  eva_dns_rr_cache_insert (rr_cache,rr,TRUE,cur_time);
  found = eva_dns_rr_cache_lookup_one (rr_cache, "foo.baz", EVA_DNS_RR_HOST_INFO, EVA_DNS_CLASS_INTERNET,
                                       EVA_DNS_RR_CACHE_LOOKUP_DEREF_CNAMES);
  g_assert (found != NULL);
  g_assert (found->type == EVA_DNS_RR_HOST_INFO);
  g_assert (found->record_class == EVA_DNS_CLASS_INTERNET);
  g_assert (strcmp (found->rdata.hinfo.os, "linux") == 0);
  g_assert (strcmp (found->rdata.hinfo.cpu, "pentium") == 0);
  eva_dns_message_unref (allocator);

  eva_dns_rr_cache_unref (rr_cache);

#define ORIGIN ""
  rr_cache = eva_dns_rr_cache_new (1024 * 1024, 1024);
  allocator = eva_dns_message_new (0, FALSE);
  rr = eva_dns_rr_text_parse ("fun.house 10000 IN CNAME extra.fun", NULL, ORIGIN, &err_msg, allocator);
  if (!rr)
    g_message ("error parsing CNAME: %s", err_msg);
  g_assert (rr != NULL);
  g_assert (rr->type == EVA_DNS_RR_CANONICAL_NAME);
  g_assert (rr->record_class == EVA_DNS_CLASS_INTERNET);
  g_assert (g_ascii_strcasecmp (rr->rdata.domain_name, "extra.fun.") == 0);
  eva_dns_rr_cache_insert (rr_cache,rr,TRUE,cur_time);
  rr = eva_dns_rr_text_parse ("extra.fun 10000 IN A 2.3.4.5", NULL, ORIGIN, &err_msg, allocator);
  if (!rr)
    g_message ("error parsing A: %s", err_msg);
  g_assert (rr != NULL);
  g_assert (rr->type == EVA_DNS_RR_HOST_ADDRESS);
  g_assert (rr->record_class == EVA_DNS_CLASS_INTERNET);
  g_assert (g_ascii_strcasecmp (rr->owner, "extra.fun.") == 0);
  eva_dns_rr_cache_insert (rr_cache,rr,TRUE,cur_time);
#if 0
  rr = eva_dns_rr_text_parse ("      10000 IN HINFO cpu os", "extra.fun", ORIGIN, &err_msg, allocator);
  if (!rr)
    g_message ("error parsing HINFO: %s", err_msg);
  g_assert (rr != NULL);
  g_assert (rr->type == EVA_DNS_RR_HOST_INFO);
  g_assert (rr->record_class == EVA_DNS_CLASS_INTERNET);
  g_assert (g_ascii_strcasecmp (rr->owner, "extra.fun.") == 0);
  eva_dns_rr_cache_insert (rr_cache,rr,TRUE,cur_time);
#endif
  rr = eva_dns_rr_text_parse ("       10000 IN MX 10 mail.host", "extra.fun.", ORIGIN, &err_msg, allocator);
  if (!rr)
    g_message ("error parsing MX: %s", err_msg);
  g_assert (rr != NULL);
  g_assert (rr->type == EVA_DNS_RR_MAIL_EXCHANGE);
  g_assert (rr->record_class == EVA_DNS_CLASS_INTERNET);
  g_assert (g_ascii_strcasecmp (rr->owner, "extra.fun.") == 0);
  g_assert (g_ascii_strcasecmp (rr->rdata.mx.mail_exchange_host_name, "mail.host.") == 0);
  g_assert (rr->rdata.mx.preference_value == 10);
  eva_dns_rr_cache_insert (rr_cache,rr,TRUE,cur_time);
  copy = eva_dns_rr_copy (rr, NULL);
  g_assert (copy->type == EVA_DNS_RR_MAIL_EXCHANGE);
  g_assert (copy->record_class == EVA_DNS_CLASS_INTERNET);
  g_assert (g_ascii_strcasecmp (copy->owner, "extra.fun.") == 0);
  g_assert (g_ascii_strcasecmp (copy->rdata.mx.mail_exchange_host_name, "mail.host.") == 0);
  g_assert (copy->rdata.mx.preference_value == 10);
  eva_dns_rr_free (copy);

  eva_dns_message_unref (allocator);

  g_assert (eva_dns_rr_cache_lookup_one (rr_cache, "negative.nelly",
					 EVA_DNS_RR_HOST_ADDRESS, EVA_DNS_CLASS_INTERNET,
                                         EVA_DNS_RR_CACHE_LOOKUP_DEREF_CNAMES) == NULL);
  g_assert (!eva_dns_rr_cache_is_negative (rr_cache, "negative.nelly",
					   EVA_DNS_RR_HOST_ADDRESS, EVA_DNS_CLASS_INTERNET));
  eva_dns_rr_cache_add_negative (rr_cache, "negative.nelly",
				 EVA_DNS_RR_HOST_ADDRESS, EVA_DNS_CLASS_INTERNET,
				 cur_time + 1000, TRUE);
  g_assert (eva_dns_rr_cache_lookup_one (rr_cache, "negative.nelly",
					 EVA_DNS_RR_HOST_ADDRESS, EVA_DNS_CLASS_INTERNET,
                                         EVA_DNS_RR_CACHE_LOOKUP_DEREF_CNAMES) == NULL);
  g_assert (eva_dns_rr_cache_is_negative (rr_cache, "negative.nelly",
					  EVA_DNS_RR_HOST_ADDRESS, EVA_DNS_CLASS_INTERNET));

  eva_dns_rr_cache_unref (rr_cache);


  return 0;
}
