#ifndef __EVA_PREFIX_TREE_H_
#define __EVA_PREFIX_TREE_H_

#include <glib.h>

G_BEGIN_DECLS

/* Private class.
 *
 * This stores a tree of prefixes,
 * for efficient prefix lookup.
 */

typedef struct _EvaPrefixTree EvaPrefixTree;
struct _EvaPrefixTree
{
  char *prefix;
  EvaPrefixTree *next_sibling;
  EvaPrefixTree *children;

  gboolean has_data;
  gpointer data;
};

/* note: a NULL EvaPrefixTree* is an empty tree. */

/* returns the pointer that we replaced, or NULL */
gpointer eva_prefix_tree_insert       (EvaPrefixTree   **tree,
                                       const char       *prefix,
                                       gpointer          data);

gpointer eva_prefix_tree_lookup       (EvaPrefixTree    *tree,
                                       const char       *str);
gpointer eva_prefix_tree_lookup_exact (EvaPrefixTree    *tree,
                                       const char       *str);
GSList  *eva_prefix_tree_lookup_all   (EvaPrefixTree    *tree,
                                       const char       *str);
gpointer eva_prefix_tree_remove       (EvaPrefixTree    *tree,
                                       const char        *prefix);
void     eva_prefix_tree_foreach      (EvaPrefixTree    *tree,
                                       GFunc              func,
                                       gpointer           func_data);
void     eva_prefix_tree_destroy      (EvaPrefixTree    *tree);


G_END_DECLS

#endif
