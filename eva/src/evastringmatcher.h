#ifndef __EVA_STRING_MATCHER_H_
#define __EVA_STRING_MATCHER_H_

typedef struct _EvaStringMatcherEntry EvaStringMatcherEntry;
typedef struct _EvaStringMatcher EvaStringMatcher;

#include <glib.h>

typedef enum
{
  EVA_STRING_MATCHER_ENTRY_FLAGS__RESERVED
} EvaStringMatcherEntryFlags;

struct _EvaStringMatcherEntry
{
  EvaStringMatcherEntryFlags flags;
  const char *str;
  gpointer entry_data;
};
  
typedef gboolean (*EvaStringMatcherFunc)(guint match_index,
                                         guint match_start_offset,
					 gpointer entry_data,
					 gpointer match_data);

EvaStringMatcher *eva_string_matcher_new (guint n_entries,
                                          EvaStringMatcherEntry *entries);

void              eva_string_matcher_run (EvaStringMatcher *matcher,
                                          const char       *haystack,
                                          EvaStringMatcherFunc match_func,
					  gpointer          match_data);

void              eva_string_matcher_free(EvaStringMatcher *matcher);

#endif
