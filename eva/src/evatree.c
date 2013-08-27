#include <stdlib.h>
#include <glib.h>
#include "evatree.h"
#include "evamacros.h"

struct _EvaTreeNode
{
  guint red : 1;
  guint is_removed : 1;
  guint visit_count : 30;
  EvaTreeNode *left;
  EvaTreeNode *right;
  EvaTreeNode *parent;
  gpointer key;
  gpointer value;
};

struct _EvaTree
{
  EvaTreeNode *top;
  GCompareDataFunc compare;
  gpointer compare_data;
  guint ref_count;
  guint n_nodes;
  guint n_alive;
  GDestroyNotify key_destroy_func;
  GDestroyNotify value_destroy_func;
};

/* 
 * Practically straight from Cormen, Leiserson, Rivest: Algorithms.
 *
 * Citations are frequent and of the form:   Algorithms:263.
 * Indeed page 263, is the start of chapter 14, Red-Black Trees.
 */

EVA_DECLARE_POOL_ALLOCATORS(EvaTree, eva_tree, 8)
EVA_DECLARE_POOL_ALLOCATORS(EvaTreeNode, eva_tree_node, 32)

static inline gboolean
is_node_red (EvaTreeNode *node)
{
  return node ? node->red : FALSE;
}

/* The following three line sed script exchanges the words `right'
 * and `left':
s/right/TMP/g;
s/left/right/g;
s/TMP/left/g;
 */

/**
 * eva_tree_new_full:
 * @compare: function to compare two keys and return -1, 0, or +1
 * to indicate their relative order.  Every distinct key
 * must compare to a nonzero value (ie. the ordering is total).
 * @compare_data: data to pass as the third argument to @compare.
 * @key_destroy_func: function to destroy a key when it is removed.
 * @value_destroy_func: function to destroy a value when it is removed.
 *
 * Create a new tree.
 *
 * returns: the new tree.
 */
EvaTree *
eva_tree_new_full  (GCompareDataFunc compare,
		    gpointer         compare_data,
		    GDestroyNotify   key_destroy_func,
		    GDestroyNotify   value_destroy_func)
{
  EvaTree *tree = eva_tree_alloc ();
  tree->compare = compare;
  tree->compare_data = compare_data;
  tree->key_destroy_func = key_destroy_func;
  tree->value_destroy_func = value_destroy_func;
  tree->ref_count = 1;
  tree->n_alive = tree->n_nodes = 0;
  tree->top = NULL;
  return tree;
}

/**
 * eva_tree_new:
 * @compare: function to compare two keys and return -1, 0, or +1
 * to indicate their relative order.  Every distinct key
 * must compare to a nonzero value (ie. the ordering is total).
 *
 * Create a new tree.
 *
 * returns: the new tree.
 */
EvaTree *
eva_tree_new (GCompareFunc     compare)
{
  return eva_tree_new_full ((GCompareDataFunc) compare, NULL, NULL, NULL);
}

/*
 * Messy internals.
 *
 * These preserve the invariants:
 *      (*) all downward paths from a node to a leaf, have the same number
 *          of black nodes (the `black-height' of a node).
 *      (*) if a node is red, then both its children are black
 *
 */

/*
 * The left and right rotations are just building blocks used in
 * `insert_fixup' and `delete_fixup'.
 * Algorithms:266.
 */
static void
eva_tree_left_rot(EvaTree     *tree,
		  EvaTreeNode *x)
{
  EvaTreeNode *y = x->right;

  x->right = y->left;
  if (y->left)
    y->left->parent = x;
  y->parent = x->parent;
  if (x->parent == NULL)
    tree->top = y;
  else if (x == x->parent->left)
    x->parent->left = y;
  else
    x->parent->right = y;
  y->left = x;
  x->parent = y;
}

static void
eva_tree_right_rot(EvaTree     *tree,
                   EvaTreeNode *x)
{
  EvaTreeNode *y = x->left;
  x->left = y->right;
  if (y->right)
    y->right->parent = x;
  y->parent = x->parent;
  if (x->parent == 0)
    tree->top = y;
  else if (x == x->parent->right)
    x->parent->right = y;
  else
    x->parent->left = y;
  y->right = x;
  x->parent = y;
}

static inline EvaTreeNode *
eva_tree_search_internal (EvaTree *tree,
			  gpointer key)
{
  EvaTreeNode *at = tree->top;
  while (at)
    {
      int c = tree->compare (at->key, key, tree->compare_data);
      if (c == 0)
	return at;
      if (c > 0)
	at = at->left;
      else
	at = at->right;
    }
  return NULL;
}

static inline EvaTreeNode *
eva_tree_node_next_internal (EvaTree      *tree,
			     EvaTreeNode  *node)
{
  EvaTreeNode *parent;
  g_return_val_if_fail (node != NULL, NULL);
  if (node->right)
    {
      node = node->right;
      while (node->left)
	node = node->left;
      return node;
    }
  parent = node->parent;
  while (parent && node == parent->right)
    {
      node = parent;
      parent = parent->parent;
    }
  return parent;
}

static inline EvaTreeNode *
eva_tree_node_prev_internal (EvaTree      *tree,
			     EvaTreeNode  *node)
{
  EvaTreeNode *parent;
  g_return_val_if_fail (node != NULL, NULL);
  if (node->left)
    {
      node = node->left;
      while (node->right)
	node = node->right;
      return node;
    }
  parent = node->parent;
  while (parent && node == parent->left)
    {
      node = parent;
      parent = parent->parent;
    }
  return parent;
}

/*
 * Fixup the red-black tree after an insertion.
 *
 * `x' starts being the element added.
 * Algorithms:268.
 */
static void
eva_tree_insert_fixup(EvaTree      *tree,
		      EvaTreeNode  *x)
{
  x->red = 1;
  while (tree->top != x && x->parent->red)
    {
      if (x->parent == x->parent->parent->left)
	{
	  EvaTreeNode *y = x->parent->parent->right;
	  if (is_node_red (y))
	    {
	      x->parent->red = 0;
	      y->red = 0;
	      x->parent->parent->red = 1;
	      x = x->parent->parent;
	    }
	  else
	    {
	      if (x == x->parent->right)
		{
		  x = x->parent;
		  eva_tree_left_rot (tree, x);
		}
	      x->parent->red = 0;
	      x->parent->parent->red = 1;
	      eva_tree_right_rot (tree, x->parent->parent);
	    }
	}
      else
	{
	  EvaTreeNode *y = x->parent->parent->left;
	  if (is_node_red (y))
	    {
	      x->parent->red = 0;
	      y->red = 0;
	      x->parent->parent->red = 1;
	      x = x->parent->parent;
	    }
	  else
	    {
	      if (x == x->parent->left)
		{
		  x = x->parent;
		  eva_tree_right_rot (tree, x);
		}
	      x->parent->red = 0;
	      x->parent->parent->red = 1;
	      eva_tree_left_rot (tree, x->parent->parent);
	    }
	}
    }
  tree->top->red = 0;
}

static EvaTreeNode *
mknode(EvaTree      *tree,
       EvaTreeNode  *parent,
       gpointer      key,
       gpointer      value,
       gboolean      is_left_child)
{
  EvaTreeNode *rv = eva_tree_node_alloc ();
  rv->key = key;
  rv->value = value;
  rv->left = rv->right = NULL;
  rv->parent = parent;
  rv->is_removed = 0;
  rv->visit_count = 0;
  /* NOTE: rv->is_red will be set by eva_tree_insert_fixup() */

  if (parent != NULL)
    {
      if (is_left_child)
	{
	  g_return_val_if_fail (parent->left == NULL, NULL);
	  parent->left = rv;
	}
      else
	{
	  g_return_val_if_fail (parent->right == NULL, NULL);
	  parent->right = rv;
	}
    }
  else
    {
      /* parent == NULL */
      tree->top = rv;
    }

  eva_tree_insert_fixup (tree, rv);
  ++(tree->n_alive);
  ++(tree->n_nodes);
  return rv;
}

static inline gint
compare_nodes (EvaTree     *tree,
	       EvaTreeNode *a,
	       EvaTreeNode *b)
{
  gint rv = (*tree->compare) (a->key, b->key, tree->compare_data);
  if (rv != 0)
    return rv;
  if (a->is_removed)
    {
      if (b->is_removed)
	return (a < b) ? -1 : (a > b) ? +1 : 0;
      else
	return +1;
    }
  else
    {
      if (b->is_removed)
	return -1;
      else
	return 0;
    }
}

/* initialize a node on the stack to have exactly 
 * enough data to call compare_nodes on it.
 */
#define INIT_SEARCH_NODE(node, key)			\
  G_STMT_START{						\
    (node).is_removed = 0;				\
    (node).key = key;					\
  }G_STMT_END

/**
 * eva_tree_insert:
 * @tree: tree to insert a new element into.
 * @key: key of the new element.
 * @value: value of the new element.
 *
 * Insert a new key/value pair into the tree.
 *
 * If the key is currently in the tree,
 * the new key is destroyed, and the old value is destroyed,
 * and the old value is replaced with the new value in the tree.
 * (This is for compatibility with g_hash_table_insert
 * and g_tree_insert.)
 *
 * See also eva_tree_replace().
 */
void
eva_tree_insert (EvaTree     *tree, 
		 gpointer     key,
		 gpointer     value)
{
  EvaTreeNode search_node;
  EvaTreeNode *parent = NULL;
  EvaTreeNode *at = tree->top;
  gboolean was_last_left = FALSE;

  INIT_SEARCH_NODE (search_node, key);

  while (at)
    {
      int c = compare_nodes (tree, &search_node, at);
      if (c == 0)
	{
	  gpointer old_value = at->value;
	  at->value = value;
	  if (tree->key_destroy_func)
	    tree->key_destroy_func (key);
	  if (tree->value_destroy_func)
	    tree->value_destroy_func (old_value);
	  return;
	}
      parent = at;
      if (c < 0)
	{
	  was_last_left = TRUE;
	  at = at->left;
	}
      else
	{
	  was_last_left = FALSE;
	  at = at->right;
	}
    }
  mknode (tree, parent, key, value, was_last_left);
}
    
/**
 * eva_tree_replace:
 * @tree: tree to insert a new element into.
 * @key: key of the new element.
 * @value: value of the new element.
 *
 * Insert a new key/value pair into the tree.
 *
 * If the key is currently in the tree,
 * the old key and value are destroyed
 * and replaced with this key.
 */
void
eva_tree_replace (EvaTree     *tree,
		  gpointer     key,
		  gpointer     value)
{
  EvaTreeNode search_node;
  EvaTreeNode *parent;
  EvaTreeNode *at;
  gboolean was_last_left = FALSE;
  INIT_SEARCH_NODE (search_node, key);
  parent = NULL;
  at = tree->top;
  while (at)
    {
      int c = compare_nodes (tree, &search_node, at);
      if (c == 0)
	{
	  gpointer old_key = at->key;
	  gpointer old_value = at->value;
	  at->key = key;
	  at->value = value;
	  if (tree->key_destroy_func)
	    tree->key_destroy_func (old_key);
	  if (tree->value_destroy_func)
	    tree->value_destroy_func (old_value);
	  return;
	}
      parent = at;
      if (c < 0)
	{
	  was_last_left = TRUE;
	  at = at->left;
	}
      else
	{
	  was_last_left = FALSE;
	  at = at->right;
	}
    }
  mknode (tree, parent, key, value, was_last_left);
}

/* Algorithms:274. */
static void
eva_tree_delete_fixup (EvaTree     *tree, 
		       EvaTreeNode *x,
		       EvaTreeNode *nullpar)
{
  while (x != tree->top && !is_node_red (x))
    {
      EvaTreeNode *xparent = x ? x->parent : nullpar;
      if (x == xparent->left)
	{
	  EvaTreeNode *w = xparent->right;
	  if (is_node_red (w))
	    {
	      w->red = 0;
	      xparent->red = 1;
	      eva_tree_left_rot (tree, xparent);
	      w = xparent->right;
	    }
	  if (!is_node_red (w->left) && !is_node_red (w->right))
	    {
	      w->red = 1;
	      x = xparent;
	    }
	  else
	    {
	      if (!is_node_red (w->right))
		{
		  if (w->left)
		    w->left->red = 0;
		  w->red = 1;
		  eva_tree_right_rot (tree, w);
		  w = xparent->right;
		}
	      w->red = xparent->red;
	      xparent->red = 0;
	      w->right->red = 0;
	      eva_tree_left_rot (tree, xparent);
	      x = tree->top;
	    }
	}
      else
	{
	  EvaTreeNode *w = xparent->left;
	  if (w->red)
	    {
	      w->red = 0;
	      xparent->red = 1;
	      eva_tree_right_rot (tree, xparent);
	      w = xparent->left;
	    }
	  if (!is_node_red (w->right) && !is_node_red (w->left))
	    {
	      w->red = 1;
	      x = xparent;
	    }
	  else
	    {
	      if (!is_node_red (w->left))
		{
		  w->right->red = 0;
		  w->red = 1;
		  eva_tree_left_rot (tree, w);
		  w = xparent->left;
		}
	      w->red = xparent->red;
	      xparent->red = 0;
	      if (w->left)
		w->left->red = 0;
	      eva_tree_right_rot (tree, xparent);
	      x = tree->top;
	    }
	}
    }
  if (x)
    x->red = 0;
}

/* Algorithms:273. */
static void
eva_tree_cut_node(EvaTree     *tree,
		  EvaTreeNode *z)
{
  EvaTreeNode *x, *y;
  EvaTreeNode *nulpar = 0;	/* Used to emulate sentinel nodes */
  int fixup;
  if (z->left == NULL || z->right == NULL)
    y = z;
  else
    y = eva_tree_node_next_internal (tree, z);
  x = y->left ? y->left : y->right;
  if (x)
    x->parent = y->parent;
  else
    nulpar = y->parent;
  if (!y->parent)
    tree->top = x;
  else
    {
      if (y == y->parent->left)
	y->parent->left = x;
      else
	y->parent->right = x;
    }
  fixup = !y->red;
  if (y != z)
    {
      y->red = z->red;
      y->left = z->left;
      y->right = z->right;
      y->parent = z->parent;
      if (y->parent)
	{
	  if (y->parent->left == z)
	    y->parent->left = y;
	  else
	    y->parent->right = y;
	}
      else
	tree->top = y;

      if (y->left)
	y->left->parent = y;
      if (y->right)
	y->right->parent = y;
      if (nulpar == z)
	nulpar = y;
    }
  if (fixup)
    eva_tree_delete_fixup (tree, x, nulpar);
  --(tree->n_nodes);
  if (!z->is_removed)
    --(tree->n_alive);
  z->left = z->right = z->parent = NULL;
}

static void
eva_tree_node_destroy (EvaTree     *tree,
		       EvaTreeNode *node)
{
  if (tree->key_destroy_func)
    tree->key_destroy_func (node->key);
  if (tree->value_destroy_func)
    tree->value_destroy_func (node->value);
  if (node->left)
    eva_tree_node_destroy (tree, node->left);
  if (node->right)
    eva_tree_node_destroy (tree, node->right);
  eva_tree_node_free (node);
}

/**
 * eva_tree_remove:
 * @tree: the tree to remove an element from.
 * @key: the key of the element to remove.
 *
 * The element with the given key will be removed from the tree.
 *
 * Note that if another iterator is visiting the node which
 * is being deleted, then the key/value pair will not
 * be destroyed until the last iterator has
 * left the node.  An iterator may determine
 * whether the node it is visiting has been removed
 * by calling eva_tree_node_is_removed().
 */
void
eva_tree_remove (EvaTree     *tree,
		 gpointer     key)
{
  EvaTreeNode *node = eva_tree_search_internal (tree, key);
  if (node == NULL)
    return;

  /* There might be more than one node with the same key, because the
   * same key might have been removed and re-added while the node was
   * being visited.  So we find the first node with the key and then
   * keep scanning forward until the key changes.
   */

  for (;;)
    {
      EvaTreeNode *prev = eva_tree_node_prev_internal (tree, node);
      if (prev == NULL ||
	  tree->compare (prev->key, key, tree->compare_data) != 0)
	break;
      node = prev;
    }

  do
    {
      EvaTreeNode *next = eva_tree_node_next_internal (tree, node);
      if (node->visit_count > 0)
	{
	  if (!node->is_removed)
	    {
	      --(tree->n_alive);
	      node->is_removed = 1;
	    }
	}
      else
	{
	  eva_tree_cut_node (tree, node);
	  eva_tree_node_destroy (tree, node);
	}
      node = next;
    }
  while (node && tree->compare (node->key, key, tree->compare_data) == 0);
}

/**
 * eva_tree_lookup:
 * @tree: the tree to query.
 * @key: the key to look up.
 *
 * Find the value of a node in the tree, given a key.
 * Or NULL if the node cannot be found.
 *
 * returns: the value of a matching node, or NULL.
 */
gpointer
eva_tree_lookup (EvaTree *tree,
		 gpointer key)
{
  EvaTreeNode *node = eva_tree_search_internal (tree, key);
  g_return_val_if_fail (!(node && node->is_removed), NULL);
  return node == NULL ? NULL : node->value;
}

/**
 * eva_tree_node_first:
 * @tree: the tree to begin iterating.
 * 
 * Return the first node in the tree,
 * and increment its visit count.
 *
 * returns: the first node in the tree, or NULL if the tree is empty.
 */

/* Algorithms:249 */
EvaTreeNode *
eva_tree_node_first (EvaTree    *tree)
{
  EvaTreeNode *node = tree->top;
  if (node == NULL)
    return NULL;
  while (node->left)
    node = node->left;
  if (node->is_removed)
    {
      node = eva_tree_node_next (tree, node);
      if (node)
	++(node->visit_count);
    }
  else
    {
      ++(node->visit_count);
    }
  return node;
}

/**
 * eva_tree_node_last:
 * @tree: the tree to begin iterating.
 * 
 * Return the last node in the tree,
 * and increment its visit count.
 *
 * returns: the last node in the tree, or NULL if the tree is empty.
 */

EvaTreeNode *
eva_tree_node_last (EvaTree    *tree)
{
  EvaTreeNode *node = tree->top;
  if (node == NULL)
    return NULL;
  while (node->right)
    node = node->right;
  if (node->is_removed)
    {
      node = eva_tree_node_prev (tree, node);
      if (node)
	++(node->visit_count);
    }
  else
    {
      ++(node->visit_count);
    }
  return node;
}
/**
 * eva_tree_node_find:
 * @tree: the tree to query.
 * @search_key: the key to look up.
 *
 * Start iterating at the given key,
 * or return NULL if the node cannot be found.
 *
 * returns: the matching node, whose visit count has been incremented,
 * or NULL.
 */
EvaTreeNode *
eva_tree_node_find (EvaTree *tree,
		    gpointer search_key)
{
  EvaTreeNode *node = eva_tree_search_internal (tree, search_key);
  g_return_val_if_fail (!(node && node->is_removed), NULL);
  if (node != NULL)
    ++(node->visit_count);
  return node;
}

/**
 * eva_tree_node_next:
 * @tree: the tree to iterate through.
 * @node: the current node.
 * 
 * Return the next node in the tree after @node,
 * and increment its visit count, while
 * decrementing the visit count of the current node.
 *
 * Usually this is used to advance an iterator:
 *   node = eva_tree_node_next(tree,node);
 *
 * returns: the next node in the tree, or NULL if @node is the last node.
 */

EvaTreeNode *
eva_tree_node_next (EvaTree *tree,
		    EvaTreeNode *node)
{
  EvaTreeNode *next;
  g_return_val_if_fail (node != NULL, NULL);
  for (next = eva_tree_node_next_internal (tree, node);
       next != NULL && next->is_removed;
       next = eva_tree_node_next_internal (tree, next))
    ;
  if (next)
    ++(next->visit_count);
  if (node)
    eva_tree_node_unvisit (tree, node);
  return next;
}

/**
 * eva_tree_node_prev:
 * @tree: the tree to iterate through.
 * @node: the current node.
 * 
 * Return the previous node in the tree after @node,
 * and increment its visit count, while
 * decrementing the visit count of the current node.
 *
 * Usually this is used to advance an iterator backward:
 *   node = eva_tree_node_prev(tree,node);
 *
 * returns: the previous node in the tree, or NULL if @node is the last node.
 */
EvaTreeNode *
eva_tree_node_prev (EvaTree *tree,
		    EvaTreeNode *node)
{
  EvaTreeNode *prev;
  g_return_val_if_fail (node != NULL, NULL);
  for (prev = eva_tree_node_prev_internal (tree, node);
       prev != NULL && prev->is_removed;
       prev = eva_tree_node_prev_internal (tree, prev))
    ;
  if (prev)
    ++(prev->visit_count);
  if (node)
    eva_tree_node_unvisit (tree, node);
  return prev;
}

/**
 * eva_tree_node_peek_key:
 * @node: visited node to query.
 *
 * Get the key of a node you are visiting.
 * It may have been removed by eva_tree_remove(),
 * but it will not have been destroyed until 
 * the visit count has been reduced to 0.
 *
 * returns: key of the current tree node.
 */
gpointer
eva_tree_node_peek_key   (EvaTreeNode     *node)
{
  return node->key;
}

/**
 * eva_tree_node_peek_value:
 * @node: visited node to query.
 *
 * Get the value of a node you are visiting.
 * It may have been removed by eva_tree_remove(),
 * but it will not have been destroyed until 
 * the visit count has been reduced to 0.
 *
 * returns: value of the current tree node.
 */
gpointer
eva_tree_node_peek_value (EvaTreeNode     *node)
{
  return node->value;
}

/**
 * eva_tree_node_is_removed:
 * @node: a node to check.
 *
 * Check to see if a node which is being visited
 * has been removed from the tree.
 *
 * (Note that if you are not visiting the node,
 * then it should already have been freed,
 * so it is only valid to call this on a node you
 * are visiting.)
 *
 * returns: whether the node has been removed.
 */
gboolean
eva_tree_node_is_removed (EvaTreeNode     *node)
{
  return node->is_removed;
}

/**
 * eva_tree_node_visit:
 * @tree: the tree that @node is iterating.
 * @node: the node in the tree that is being visited.
 *
 * Increment the visit count for a node.
 * A non-zero visit count prevents the
 * node's key and value from being deleted,
 * even after the node is "removed" from the tree.
 *
 * The removed node may still be used to advance to "next"
 * and "prev", however, you may iterate freely
 * from a "removed" node, and you cannot get back to it.
 */
void
eva_tree_node_visit (EvaTree     *tree,
		     EvaTreeNode *node)
{
  g_return_if_fail (node->visit_count > 0);
  ++(node->visit_count);
}

/**
 * eva_tree_node_unvisit:
 * @tree: the tree that @node is iterating.
 * @node: the node in the tree which you want to stop visiting.
 *
 * Decrease the visit count on the node,
 * indicating that you are done visiting it.
 *
 * If the node has been removed and you
 * are the last visitor, its key and value will be deleted at this point.
 */
void
eva_tree_node_unvisit (EvaTree     *tree,
		       EvaTreeNode *node)
{
  g_return_if_fail (node->visit_count > 0);
  if (--(node->visit_count) == 0 && node->is_removed)
    {
      eva_tree_cut_node (tree, node);
      eva_tree_node_destroy (tree, node);
    }
}

/**
 * eva_tree_clear:
 * @tree: the tree to destroy.
 *
 * Destroy the tree and all its nodes.
 *
 * Any EvaTreeNode's that you are visiting are destroyed immediately!
 */
void
eva_tree_clear (EvaTree    *tree)
{
  if (tree->top)
    {
      EvaTreeNode *top = tree->top;
      tree->top = NULL;
      eva_tree_node_destroy (tree, top);
      tree->n_alive = tree->n_nodes = 0;
    }
}

/**
 * eva_tree_unref:
 * @tree: tree to release your reference to.
 *
 * Decrease the reference count on the tree.
 * Once its ref-count reached 0, it and all its nodes
 * will be destroyed.  So it may be a good idea
 * to hold a reference to a tree you are visiting!
 */
void
eva_tree_unref (EvaTree   *tree)
{
  g_return_if_fail (tree->ref_count > 0);
  if (--(tree->ref_count) == 0)
    {
      eva_tree_clear (tree);
      eva_tree_free (tree);
    }
}

/**
 * eva_tree_ref:
 * @tree: tree to add a reference to.
 *
 * Increase the reference count on the tree.
 *
 * returns: the @tree, as a convenience; it leads to nice looking code.
 */
EvaTree *
eva_tree_ref (EvaTree   *tree)
{
  g_return_val_if_fail (tree->ref_count > 0, NULL);
  ++(tree->ref_count);
  return tree;
}

/**
 * eva_tree_n_nodes:
 * @tree: the tree to query.
 *
 * Get the size of the tree, not including removed nodes, even
 * if they are currently being visited.
 *
 * returns: the number of nodes in the tree.
 */
guint
eva_tree_n_nodes (EvaTree *tree)
{
  return tree->n_nodes;
}

/* Returns black height, or -1 if bad. */
static int
eva_tree_node_check (EvaTree* tree,
		     EvaTreeNode *n,
		     EvaTreeNode **min_out,
		     EvaTreeNode **max_out)
{
  int black_height = 0;
  if (n->left)
    {
      EvaTreeNode *tmp_max;
      black_height = eva_tree_node_check (tree, n->left, min_out, &tmp_max);
      if (black_height == -1)
	return -1;
      if (compare_nodes (tree, tmp_max, n) >= 0)
	{
	  /*fprintf(stderr, "left tree's max <= this node\n"); */
	  return -1;
	}
    }
  if (black_height < 0)
    return -1;
  if (n->right)
    {
      EvaTreeNode *tmp_min;
      int bh2 = eva_tree_node_check (tree, n->right, &tmp_min, max_out);
      if (bh2 == -1)
	return -1;
      if (n->left && black_height != bh2)
	{
	  /*fprintf(stderr, "black height's don't match\n"); */
	  return -1;
	}
      if (compare_nodes (tree, n, tmp_min) >= 0)
	{
	  /*fprintf(stderr, "right tree's min >= this node\n"); */
	  return -1;
	}
      black_height = bh2;
    }
  if (!n->left && min_out)
    *min_out = n;
  if (!n->right && max_out)
    *max_out = n;
  if (n->red && ((n->left && n->left->red) || (n->right && n->right->red)))
    {
      /*fprintf(stderr, "red node with black children\n"); */
      return -1;
    }
  if (!n->red)
    black_height++;
  return black_height;
}

/**
 * eva_tree_validate:
 * @tree: the tree to check.
 *
 * Check the tree for integrity.
 * This is only used for debugging.
 *
 * returns: whether the tree was valid.
 * If it returns FALSE, then there is either a
 * bug in the tree code, or the comparison function
 * isn't transitive.
 */
gboolean
eva_tree_validate (EvaTree *tree)
{
  if (tree->top == NULL)
    return TRUE;
  return eva_tree_node_check (tree, tree->top, NULL, NULL) != -1;
}
