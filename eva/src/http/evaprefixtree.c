#include "evaprefixtree.h"
#include <string.h>

static inline EvaPrefixTree *
tree_alloc (const char *str)
{
  EvaPrefixTree *rv = g_new (EvaPrefixTree, 1);
  rv->prefix = g_strdup (str);
  rv->next_sibling = rv->children = NULL;
  rv->has_data = FALSE;
  return rv;
}

static inline EvaPrefixTree *
tree_alloc_n (const char *str, guint len)
{
  EvaPrefixTree *rv = g_new (EvaPrefixTree, 1);
  rv->prefix = g_strndup (str, len);
  rv->next_sibling = rv->children = NULL;
  rv->has_data = FALSE;
  return rv;
}
static inline void
tree_set_prefix (EvaPrefixTree *tree,
                  const char *prefix)
{
  char *dup = g_strdup (prefix);
  g_free (tree->prefix);
  tree->prefix = dup;
}

gpointer eva_prefix_tree_insert       (EvaPrefixTree   **tree,
                                       const char       *prefix,
                                       gpointer          data)
{
  g_return_val_if_fail (prefix[0] != 0, NULL);

  /* look for a node that shares some prefix in common with us */
  while (*tree)
    {
      if (prefix[0] == (*tree)->prefix[0])
        {
          const char *tp_at = (*tree)->prefix;
          const char *p_at = prefix;
          while (*p_at != 0 && *p_at == *tp_at)
            {
              p_at++;
              tp_at++;
            }
          if (*p_at == 0 && *tp_at == 0)
            {
              /* *tree is an exact match */
              gpointer rv = (*tree)->has_data ? (*tree)->data : NULL;
              (*tree)->has_data = TRUE;
              (*tree)->data = data;
              return rv;
            }

          if (*tp_at == 0)      /* p_at not empty */
            {
              tree = &((*tree)->children);
              prefix = p_at;
              continue;
            }

          if (*p_at == 0)
            {
              /* pattern is too short for tree: must restructure tree:
                  
                  if tree is 'prevsib's' pointer, and *tree is the crrent node,
                  then
                    
                     prevsib                         prevsib
                       |                                |
                       v                                v
                     *tree -> child        becomes     new  -> *tree  -> child
                       |                                |
                       v                                v
                     nextsib                         nextsib */
              EvaPrefixTree *new = tree_alloc_n (prefix, p_at - prefix);
              new->next_sibling = (*tree)->next_sibling;
              (*tree)->next_sibling = NULL;
              new->children = (*tree);
              tree_set_prefix (new->children, tp_at);
              *tree = new;
              new->has_data = TRUE;
              new->data = data;
              return NULL;
            }

          /* else, a mismatch.  we must make a new family of trees.

                  prevsib                   prevsib
                     |                         |
                     v                         v
                   *tree       becomes      common -> *tree(end)
                     |                         |         |
                     v                         v         v
                   nextsib                   nextsib    new */
          {
            EvaPrefixTree *common = tree_alloc_n (prefix, p_at - prefix);
            EvaPrefixTree *cur = *tree;
            common->next_sibling = cur->next_sibling;
            common->children = cur;
            *tree = common;
            cur->next_sibling = NULL;
            tree = &(cur->next_sibling);
            tree_set_prefix (cur, tp_at);
            prefix += p_at - prefix;

            /* prefix not empty */
          }
        }
      else
        {
          tree = &((*tree)->next_sibling);
        }
    }

  /* create a new node with the remaining characters is in */
  *tree = tree_alloc (prefix);
  (*tree)->has_data = TRUE;
  (*tree)->data = data;
  return NULL;
}


gpointer eva_prefix_tree_lookup       (EvaPrefixTree    *tree,
                                       const char       *str)
{
  gpointer found = NULL;
  EvaPrefixTree *at = tree;

  while (*str && at)
    {
      for (         ; at; at = at->next_sibling)
        if (g_str_has_prefix (str, at->prefix))
          {
            str += strlen (at->prefix);
            if (at->has_data)
              found = at->data;
            at = at->children;
            break;
          }
    }
  return found;
}

gpointer eva_prefix_tree_lookup_exact (EvaPrefixTree    *tree,
                                       const char       *str)
{
  gpointer found = NULL;
  EvaPrefixTree *at = tree;

  while (*str && at)
    {
      for (         ; at; at = at->next_sibling)
        if (g_str_has_prefix (str, at->prefix))
          {
            str += strlen (at->prefix);
            if (at->has_data)
              found = at->data;
            at = at->children;
            break;
          }
    }
  return *str ? NULL : found;
}

GSList  *eva_prefix_tree_lookup_all   (EvaPrefixTree    *tree,
                                       const char       *str)
{
  GSList *rv = NULL;
  EvaPrefixTree *at = tree;

  while (*str && at)
    {
      for (         ; at; at = at->next_sibling)
        if (g_str_has_prefix (str, at->prefix))
          {
            str += strlen (at->prefix);
            if (at->has_data)
              rv = g_slist_prepend (rv, at->data);
            at = at->children;
            break;
          }
    }
  return rv;
}
// good idea, but we don't need it.
//gpointer eva_prefix_tree_remove       (EvaPrefixTree    *tree,
//                                       const char        *prefix)
//{
//  ...
//}
//
void     eva_prefix_tree_foreach      (EvaPrefixTree    *tree,
                                       GFunc              func,
                                       gpointer           func_data)
{
  while (tree)
    {
      if (tree->has_data)
        func (tree->data, func_data);
      eva_prefix_tree_foreach (tree->children, func, func_data);
      tree = tree->next_sibling;
    }
}
void     eva_prefix_tree_destroy      (EvaPrefixTree    *tree)
{
  while (tree)
    {
      EvaPrefixTree *next = tree->next_sibling;
      g_free (tree->prefix);
      eva_prefix_tree_destroy (tree->children);
      tree = next;
    }
}
