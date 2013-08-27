#include "../evatree.h"

static gint
direct_compare (gconstpointer a, gconstpointer b)
{
  guint a_int = GPOINTER_TO_UINT (a);
  guint b_int = GPOINTER_TO_UINT (b);
  if (a_int < b_int) return -1;
  if (a_int > b_int) return +1;
  return 0;
}

#define ASSERT_NODE_VALUES(a,b)							\
	g_assert (node != NULL);						\
	g_assert (eva_tree_node_peek_key (node) == GUINT_TO_POINTER (a));	\
	g_assert (eva_tree_node_peek_value (node) == GUINT_TO_POINTER (b));	\

int main ()
{
  GskTree *tree;
  GskTreeNode *node;

  /* TEST1.0: 1 element tree */
  tree = eva_tree_new (direct_compare);
  g_assert (eva_tree_validate (tree));
  eva_tree_insert (tree, GUINT_TO_POINTER (2), GUINT_TO_POINTER (4));
  g_assert (eva_tree_validate (tree));
  node = eva_tree_node_first (tree);
  ASSERT_NODE_VALUES (2, 4);
  node = eva_tree_node_next (tree, node);
  g_assert (node == NULL);

  /* TEST1.1: 3 element tree */
  eva_tree_insert (tree, GUINT_TO_POINTER (1), GUINT_TO_POINTER (2));
  g_assert (eva_tree_validate (tree));
  eva_tree_insert (tree, GUINT_TO_POINTER (3), GUINT_TO_POINTER (6));
  g_assert (eva_tree_validate (tree));
  node = eva_tree_node_first (tree);
  ASSERT_NODE_VALUES (1, 2);
  node = eva_tree_node_next (tree, node);
  ASSERT_NODE_VALUES (2, 4);
  node = eva_tree_node_next (tree, node);
  ASSERT_NODE_VALUES (3, 6);
  node = eva_tree_node_next (tree, node);
  g_assert (node == NULL);
  node = eva_tree_node_last (tree);
  ASSERT_NODE_VALUES (3, 6);
  node = eva_tree_node_prev (tree, node);
  ASSERT_NODE_VALUES (2, 4);
  node = eva_tree_node_prev (tree, node);
  ASSERT_NODE_VALUES (1, 2);
  node = eva_tree_node_prev (tree, node);
  g_assert (node == NULL);

  g_assert (GPOINTER_TO_UINT (eva_tree_lookup (tree, GUINT_TO_POINTER (1))) == 2);
  g_assert (GPOINTER_TO_UINT (eva_tree_lookup (tree, GUINT_TO_POINTER (2))) == 4);
  g_assert (GPOINTER_TO_UINT (eva_tree_lookup (tree, GUINT_TO_POINTER (3))) == 6);
  eva_tree_remove (tree, GUINT_TO_POINTER (2));
  g_assert (eva_tree_validate (tree));
  g_assert (GPOINTER_TO_UINT (eva_tree_lookup (tree, GUINT_TO_POINTER (1))) == 2);
  g_assert (eva_tree_lookup (tree, GUINT_TO_POINTER (2)) == NULL);
  g_assert (GPOINTER_TO_UINT (eva_tree_lookup (tree, GUINT_TO_POINTER (3))) == 6);
  eva_tree_remove (tree, GUINT_TO_POINTER (3));
  g_assert (GPOINTER_TO_UINT (eva_tree_lookup (tree, GUINT_TO_POINTER (1))) == 2);
  g_assert (eva_tree_lookup (tree, GUINT_TO_POINTER (2)) == NULL);
  g_assert (eva_tree_lookup (tree, GUINT_TO_POINTER (3)) == NULL);
  g_assert (eva_tree_validate (tree));
  eva_tree_remove (tree, GUINT_TO_POINTER (1));
  g_assert (eva_tree_lookup (tree, GUINT_TO_POINTER (1)) == NULL);
  g_assert (eva_tree_lookup (tree, GUINT_TO_POINTER (2)) == NULL);
  g_assert (eva_tree_lookup (tree, GUINT_TO_POINTER (3)) == NULL);
  g_assert (eva_tree_validate (tree));


  /* TEST1.2: destruction */
  eva_tree_unref (tree);

  /* TEST2.0: 3 element tree, remove during iteration */
  tree = eva_tree_new (direct_compare);
  g_assert (eva_tree_validate (tree));
  eva_tree_insert (tree, GUINT_TO_POINTER (9), GUINT_TO_POINTER (1));
  g_assert (eva_tree_validate (tree));
  eva_tree_insert (tree, GUINT_TO_POINTER (7), GUINT_TO_POINTER (2));
  g_assert (eva_tree_validate (tree));
  eva_tree_insert (tree, GUINT_TO_POINTER (8), GUINT_TO_POINTER (3));
  g_assert (eva_tree_validate (tree));
  node = eva_tree_node_first (tree);
  ASSERT_NODE_VALUES (7, 2);
  node = eva_tree_node_next (tree, node);
  ASSERT_NODE_VALUES (8, 3);
  g_assert (eva_tree_n_nodes (tree) == 3);
  eva_tree_remove (tree, GUINT_TO_POINTER (8));
  g_assert (eva_tree_node_is_removed (node));
  node = eva_tree_node_next (tree, node);
  ASSERT_NODE_VALUES (9, 1);
  g_assert (eva_tree_n_nodes (tree) == 2);
  node = eva_tree_node_next (tree, node);
  g_assert (node == NULL);
  eva_tree_unref (tree);

  /* TEST2.1: 3 element tree, remove/insert during iteration */
  tree = eva_tree_new (direct_compare);
  g_assert (eva_tree_validate (tree));
  eva_tree_insert (tree, GUINT_TO_POINTER (9), GUINT_TO_POINTER (1));
  g_assert (eva_tree_validate (tree));
  eva_tree_insert (tree, GUINT_TO_POINTER (7), GUINT_TO_POINTER (2));
  g_assert (eva_tree_validate (tree));
  eva_tree_insert (tree, GUINT_TO_POINTER (8), GUINT_TO_POINTER (3));
  g_assert (eva_tree_validate (tree));
  node = eva_tree_node_first (tree);
  ASSERT_NODE_VALUES (7, 2);
  node = eva_tree_node_next (tree, node);
  ASSERT_NODE_VALUES (8, 3);
  g_assert (eva_tree_n_nodes (tree) == 3);
  eva_tree_remove (tree, GUINT_TO_POINTER (8));
  g_assert (eva_tree_validate (tree));
  g_assert (eva_tree_node_is_removed (node));
  eva_tree_insert (tree, GUINT_TO_POINTER (8), GUINT_TO_POINTER (5));
  g_assert (eva_tree_validate (tree));
  g_assert (eva_tree_node_is_removed (node));
  node = eva_tree_node_next (tree, node);
  g_assert (!eva_tree_node_is_removed (node));
  g_assert (eva_tree_n_nodes (tree) == 3);
  eva_tree_node_unvisit (tree, node);
  node = eva_tree_node_first (tree);
  ASSERT_NODE_VALUES (7, 2);
  g_assert (!eva_tree_node_is_removed (node));
  node = eva_tree_node_next (tree, node);
  ASSERT_NODE_VALUES (8, 5);
  g_assert (!eva_tree_node_is_removed (node));
  node = eva_tree_node_next (tree, node);
  ASSERT_NODE_VALUES (9, 1);
  g_assert (!eva_tree_node_is_removed (node));
  node = eva_tree_node_next (tree, node);
  g_assert (node == NULL);
  eva_tree_unref (tree);

  /* TEST2.2: 3 element tree, remove/insert/remove during iteration */
  tree = eva_tree_new (direct_compare);
  g_assert (eva_tree_validate (tree));
  eva_tree_insert (tree, GUINT_TO_POINTER (9), GUINT_TO_POINTER (2));
  g_assert (eva_tree_validate (tree));
  eva_tree_insert (tree, GUINT_TO_POINTER (7), GUINT_TO_POINTER (3));
  g_assert (eva_tree_validate (tree));
  eva_tree_insert (tree, GUINT_TO_POINTER (8), GUINT_TO_POINTER (4));
  g_assert (eva_tree_validate (tree));
  node = eva_tree_node_first (tree);
  ASSERT_NODE_VALUES (7, 3);
  node = eva_tree_node_next (tree, node);
  ASSERT_NODE_VALUES (8, 4);
  g_assert (eva_tree_n_nodes (tree) == 3);
  eva_tree_remove (tree, GUINT_TO_POINTER (8));
  g_assert (eva_tree_validate (tree));
  g_assert (eva_tree_node_is_removed (node));
  eva_tree_insert (tree, GUINT_TO_POINTER (8), GUINT_TO_POINTER (5));
  g_assert (eva_tree_validate (tree));
  g_assert (eva_tree_node_is_removed (node));
  eva_tree_remove (tree, GUINT_TO_POINTER (8));
  g_assert (eva_tree_validate (tree));
  eva_tree_node_unvisit (tree, node);
  g_assert (eva_tree_validate (tree));
  g_assert (eva_tree_n_nodes (tree) == 2);

  node = eva_tree_node_first (tree);
  g_assert (!eva_tree_node_is_removed (node));
  ASSERT_NODE_VALUES (7, 3);
  node = eva_tree_node_next (tree, node);
  g_assert (!eva_tree_node_is_removed (node));
  ASSERT_NODE_VALUES (9, 2);
  node = eva_tree_node_next (tree, node);
  g_assert (node == NULL);
  eva_tree_unref (tree);

  return 0;
}
