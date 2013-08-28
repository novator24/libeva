#ifndef __EVA_RBTREE_MACROS_H_
#define __EVA_RBTREE_MACROS_H_

/* Macros for construction of a red-black tree.
 * Like our other macro-based data-structures,
 * this doesn't allocate memory, and it doesn't
 * use any helper functions.
 *
 * It supports "invasive" tree structures,
 * for example, you can have nodes that are members
 * of two different trees.
 *
 * A rbtree is a tuple:
 *    top
 *    type*
 *    is_red
 *    set_is_red
 *    parent
 *    left
 *    right
 *    comparator
 * that should be defined by a macro like "GET_TREE()".
 * See tests/test-rbtree-macros.
 *
 * The methods are:
 *   INSERT(tree, node, collision_node) 
 *         Try to insert 'node' into tree.
 *         If an equivalent node exists,
 *         return it in collision_node, and do nothing else.
 *         Otherwise, node is added to the tree
 *         (and collision_node is set to NULL).
 *   REMOVE(tree, node)
 *         Remove a node from the tree.
 *   LOOKUP(tree, node, out)
 *         Find a node in the tree.
 *         Finds the node equivalent with 
 *         'node', and returns it in 'out',
 *         or sets out to NULL if no node exists.
 *   LOOKUP_COMPARATOR(tree, key, key_comparator, out)
 *         Find a node in the tree, based on a key
 *         which may not be in the same format as the node.
 *   FIRST(tree, out)
 *         Set 'out' to the first node in the tree.
 *   LAST(tree, out)
 *         Set 'out' to the last node in the tree.
 *   PREV(tree, cur, out)
 *         Set 'out' to the previous node in the tree before cur.
 *   NEXT(tree, cur, out)
 *         Set 'out' to the next node in the tree after cur.
 *   INFIMUM_COMPARATOR(tree, key, key_comparator, out)
 *         Find the last node in the tree which is before or equal to 'key'.
 *   SUPREMUM_COMPARATOR(tree, key, key_comparator, out)
 *         Find the first node in the tree which is after or equal to 'key'.
 *   INFIMUM(tree, key, out)
 *         Find the last node in the tree which is before or equal to 'key'.
 *   SUPREMUM(tree, key, out)
 *         Find the first node in the tree which is after or equal to 'key'.
 *
 * Occasionally, you may need the "RBCTREE", which has the is_red flag,
 * but also keeps a "count" field at every node telling the size of that subtree.
 * This then has all the methods of RBTREE, plus:
 *   GET_BY_INDEX(tree, n, out)
 *         Return the n-th element in the tree.
 *   GET_BY_INDEX_UNCHECKED(tree, n, out)
 *         Return the n-th element in the tree;
 *         n MUST be less than or equal to the number of
 *         nodes in the tree.
 *   GET_NODE_INDEX(tree, node, n_out)
 *        Sets n_out to the index of node in the tree.
 *   
 * An rbctree is a tuple:
 *    top
 *    type*
 *    is_red
 *    set_is_red
 *    get_count
 *    set_count
 *    parent
 *    left
 *    right
 *    comparator
 */

/*
 * By and large, the red-black tree algorithms used here are from
 * the classic text:
 *     _Introduction to Algorithms_. Thomas Cormen, Charles Leiserson,
 *     and Donald Rivest.  MIT Press.  1990.
 * Citations appears as Algorithms:300 indicating page 300 (for example).
 * The "rbctree" is my name for this idea (daveb),
 * which i suspect has been thought of and implemented before.
 */
#define EVA_RBTREE_INSERT(tree, node, collision_node)                         \
  EVA_RBTREE_INSERT_(tree, node, collision_node)
#define EVA_RBTREE_REMOVE(tree, node)                                         \
  EVA_RBTREE_REMOVE_(tree, node)
#define EVA_RBTREE_LOOKUP(tree, key, out)                                     \
  EVA_RBTREE_LOOKUP_(tree, key, out)
#define EVA_RBTREE_LOOKUP_COMPARATOR(tree, key, key_comparator, out)          \
  EVA_RBTREE_LOOKUP_COMPARATOR_(tree, key, key_comparator, out)

#define EVA_RBTREE_FIRST(tree, out)                                           \
  EVA_RBTREE_FIRST_(tree, out)
#define EVA_RBTREE_LAST(tree, out)                                            \
  EVA_RBTREE_LAST_(tree, out)
#define EVA_RBTREE_NEXT(tree, in, out)                                        \
  EVA_RBTREE_NEXT_(tree, in, out)
#define EVA_RBTREE_PREV(tree, in, out)                                        \
  EVA_RBTREE_PREV_(tree, in, out)

#define EVA_RBTREE_SUPREMUM(tree, key, out)                                   \
  EVA_RBTREE_SUPREMUM_(tree, key, out)
#define EVA_RBTREE_SUPREMUM_COMPARATOR(tree, key, key_comparator, out)        \
  EVA_RBTREE_SUPREMUM_COMPARATOR_(tree, key, key_comparator, out)
#define EVA_RBTREE_INFIMUM(tree, key, out)                                    \
  EVA_RBTREE_INFIMUM_(tree, key, out)
#define EVA_RBTREE_INFIMUM_COMPARATOR(tree, key, key_comparator, out)         \
  EVA_RBTREE_INFIMUM_COMPARATOR_(tree, key, key_comparator, out)

#if 1
#undef G_STMT_START
#define G_STMT_START do
#undef G_STMT_END
#define G_STMT_END while(0)
#endif

#define EVA_RBTREE_INSERT_(top,type,is_red,set_is_red,parent,left,right,comparator, node,collision_node) \
G_STMT_START{                                                                 \
  type _eva_last = NULL;                                                      \
  type _eva_at = (top);                                                       \
  gboolean _eva_last_was_left = FALSE;                                        \
  collision_node = NULL;                                                      \
  while (_eva_at != NULL)                                                     \
    {                                                                         \
      int _eva_compare_rv;                                                    \
      _eva_last = _eva_at;                                                    \
      comparator(_eva_at, (node), _eva_compare_rv);                           \
      if (_eva_compare_rv > 0)                                                \
        {                                                                     \
          _eva_last_was_left = TRUE;                                          \
          _eva_at = _eva_at->left;                                            \
        }                                                                     \
      else if (_eva_compare_rv < 0)                                           \
        {                                                                     \
          _eva_last_was_left = FALSE;                                         \
          _eva_at = _eva_at->right;                                           \
        }                                                                     \
      else                                                                    \
        break;                                                                \
   }                                                                          \
  if (_eva_at != NULL)                                                        \
    {                                                                         \
      /* collision */                                                         \
      collision_node = _eva_at;                                               \
    }                                                                         \
  else if (_eva_last == NULL)                                                 \
    {                                                                         \
      /* only node in tree */                                                 \
      top = (node);                                                           \
      set_is_red ((node), 0);                                                 \
      (node)->left = (node)->right = (node)->parent = NULL;                   \
    }                                                                         \
  else                                                                        \
    {                                                                         \
      (node)->parent = _eva_last;                                             \
      (node)->left = (node)->right = NULL;                                    \
      if (_eva_last_was_left)                                                 \
        _eva_last->left = (node);                                             \
      else                                                                    \
        _eva_last->right = (node);                                            \
                                                                              \
      /* fixup */                                                             \
      _eva_at = (node);                                                       \
      set_is_red (_eva_at, 1);                                                \
      while (top != _eva_at && is_red(_eva_at->parent))                       \
        {                                                                     \
          if (_eva_at->parent == _eva_at->parent->parent->left)               \
            {                                                                 \
              type _eva_y = _eva_at->parent->parent->right;                   \
              if (_eva_y != NULL && is_red (_eva_y))                          \
                {                                                             \
                  set_is_red (_eva_at->parent, 0);                            \
                  set_is_red (_eva_y, 0);                                     \
                  set_is_red (_eva_at->parent->parent, 1);                    \
                  _eva_at = _eva_at->parent->parent;                          \
                }                                                             \
              else                                                            \
                {                                                             \
                  if (_eva_at == _eva_at->parent->right)                      \
                    {                                                         \
                      _eva_at = _eva_at->parent;                              \
                      EVA_RBTREE_ROTATE_LEFT (top,type,parent,left,right, _eva_at);\
                    }                                                         \
                  set_is_red(_eva_at->parent, 0);                             \
                  set_is_red(_eva_at->parent->parent, 1);                     \
                  EVA_RBTREE_ROTATE_RIGHT (top,type,parent,left,right,        \
                                           _eva_at->parent->parent);          \
                }                                                             \
            }                                                                 \
          else                                                                \
            {                                                                 \
              type _eva_y = _eva_at->parent->parent->left;                    \
              if (_eva_y != NULL && is_red (_eva_y))                          \
                {                                                             \
                  set_is_red (_eva_at->parent, 0);                            \
                  set_is_red (_eva_y, 0);                                     \
                  set_is_red (_eva_at->parent->parent, 1);                    \
                  _eva_at = _eva_at->parent->parent;                          \
                }                                                             \
              else                                                            \
                {                                                             \
                  if (_eva_at == _eva_at->parent->left)                       \
                    {                                                         \
                      _eva_at = _eva_at->parent;                              \
                      EVA_RBTREE_ROTATE_RIGHT (top,type,parent,left,right,    \
                                               _eva_at);                      \
                    }                                                         \
                  set_is_red(_eva_at->parent, 0);                             \
                  set_is_red(_eva_at->parent->parent, 1);                     \
                  EVA_RBTREE_ROTATE_LEFT (top,type,parent,left,right,         \
                                          _eva_at->parent->parent);           \
                }                                                             \
            }                                                                 \
        }                                                                     \
      set_is_red((top), 0);                                                   \
    }                                                                         \
}G_STMT_END

#define EVA_RBTREE_REMOVE_(top,type,is_red,set_is_red,parent,left,right,comparator, node) \
/* Algorithms:273. */                                                         \
G_STMT_START{                                                                 \
  type _eva_rb_del_z = (node);                                                \
  type _eva_rb_del_x;                                                         \
  type _eva_rb_del_y;                                                         \
  type _eva_rb_del_nullpar = NULL;	/* Used to emulate sentinel nodes */  \
  gboolean _eva_rb_del_fixup;                                                 \
  if (_eva_rb_del_z->left == NULL || _eva_rb_del_z->right == NULL)            \
    _eva_rb_del_y = _eva_rb_del_z;                                            \
  else                                                                        \
    {                                                                         \
      EVA_RBTREE_NEXT_ (top,type,is_red,set_is_red,parent,left,right,comparator,\
                        _eva_rb_del_z, _eva_rb_del_y);                        \
    }                                                                         \
  _eva_rb_del_x = _eva_rb_del_y->left ? _eva_rb_del_y->left                   \
                                            : _eva_rb_del_y->right;           \
  if (_eva_rb_del_x)                                                          \
    _eva_rb_del_x->parent = _eva_rb_del_y->parent;                            \
  else                                                                        \
    _eva_rb_del_nullpar = _eva_rb_del_y->parent;                              \
  if (!_eva_rb_del_y->parent)                                                 \
    top = _eva_rb_del_x;                                                      \
  else                                                                        \
    {                                                                         \
      if (_eva_rb_del_y == _eva_rb_del_y->parent->left)                       \
	_eva_rb_del_y->parent->left = _eva_rb_del_x;                          \
      else                                                                    \
	_eva_rb_del_y->parent->right = _eva_rb_del_x;                         \
    }                                                                         \
  _eva_rb_del_fixup = !is_red(_eva_rb_del_y);                                 \
  if (_eva_rb_del_y != _eva_rb_del_z)                                         \
    {                                                                         \
      set_is_red(_eva_rb_del_y, is_red(_eva_rb_del_z));                       \
      _eva_rb_del_y->left = _eva_rb_del_z->left;                              \
      _eva_rb_del_y->right = _eva_rb_del_z->right;                            \
      _eva_rb_del_y->parent = _eva_rb_del_z->parent;                          \
      if (_eva_rb_del_y->parent)                                              \
	{                                                                     \
	  if (_eva_rb_del_y->parent->left == _eva_rb_del_z)                   \
	    _eva_rb_del_y->parent->left = _eva_rb_del_y;                      \
	  else                                                                \
	    _eva_rb_del_y->parent->right = _eva_rb_del_y;                     \
	}                                                                     \
      else                                                                    \
	top = _eva_rb_del_y;                                                  \
                                                                              \
      if (_eva_rb_del_y->left)                                                \
	_eva_rb_del_y->left->parent = _eva_rb_del_y;                          \
      if (_eva_rb_del_y->right)                                               \
	_eva_rb_del_y->right->parent = _eva_rb_del_y;                         \
      if (_eva_rb_del_nullpar == _eva_rb_del_z)                               \
	_eva_rb_del_nullpar = _eva_rb_del_y;                                  \
    }                                                                         \
  if (_eva_rb_del_fixup)                                                      \
    {                                                                         \
      /* delete fixup (Algorithms, p 274) */                                  \
      while (_eva_rb_del_x != top                                             \
         && !(_eva_rb_del_x != NULL && is_red (_eva_rb_del_x)))               \
        {                                                                     \
          type _eva_rb_del_xparent = _eva_rb_del_x ? _eva_rb_del_x->parent    \
                                                   : _eva_rb_del_nullpar;     \
          if (_eva_rb_del_x == _eva_rb_del_xparent->left)                     \
            {                                                                 \
              type _eva_rb_del_w = _eva_rb_del_xparent->right;                \
              if (_eva_rb_del_w != NULL && is_red (_eva_rb_del_w))            \
                {                                                             \
                  set_is_red (_eva_rb_del_w, 0);                              \
                  set_is_red (_eva_rb_del_xparent, 1);                        \
                  EVA_RBTREE_ROTATE_LEFT (top,type,parent,left,right,         \
                                          _eva_rb_del_xparent);               \
                  _eva_rb_del_w = _eva_rb_del_xparent->right;                 \
                }                                                             \
              if (!(_eva_rb_del_w->left && is_red (_eva_rb_del_w->left))      \
               && !(_eva_rb_del_w->right && is_red (_eva_rb_del_w->right)))   \
                {                                                             \
                  set_is_red (_eva_rb_del_w, 1);                              \
                  _eva_rb_del_x = _eva_rb_del_xparent;                        \
                }                                                             \
              else                                                            \
                {                                                             \
                  if (!(_eva_rb_del_w->right && is_red (_eva_rb_del_w->right)))\
                    {                                                         \
                      if (_eva_rb_del_w->left)                                \
                        set_is_red (_eva_rb_del_w->left, 0);                  \
                      set_is_red (_eva_rb_del_w, 1);                          \
                      EVA_RBTREE_ROTATE_RIGHT (top,type,parent,left,right,    \
                                               _eva_rb_del_w);                \
                      _eva_rb_del_w = _eva_rb_del_xparent->right;             \
                    }                                                         \
                  set_is_red (_eva_rb_del_w, is_red (_eva_rb_del_xparent));   \
                  set_is_red (_eva_rb_del_xparent, 0);                        \
                  set_is_red (_eva_rb_del_w->right, 0);                       \
                  EVA_RBTREE_ROTATE_LEFT (top,type,parent,left,right,         \
                                          _eva_rb_del_xparent);               \
                  _eva_rb_del_x = top;                                        \
                }                                                             \
            }                                                                 \
          else                                                                \
            {                                                                 \
              type _eva_rb_del_w = _eva_rb_del_xparent->left;                 \
              if (_eva_rb_del_w && is_red (_eva_rb_del_w))                    \
                {                                                             \
                  set_is_red (_eva_rb_del_w, 0);                              \
                  set_is_red (_eva_rb_del_xparent, 1);                        \
                  EVA_RBTREE_ROTATE_RIGHT (top,type,parent,left,right,        \
                                           _eva_rb_del_xparent);              \
                  _eva_rb_del_w = _eva_rb_del_xparent->left;                  \
                }                                                             \
              if (!(_eva_rb_del_w->right && is_red (_eva_rb_del_w->right))    \
               && !(_eva_rb_del_w->left && is_red (_eva_rb_del_w->left)))     \
                {                                                             \
                  set_is_red (_eva_rb_del_w, 1);                              \
                  _eva_rb_del_x = _eva_rb_del_xparent;                        \
                }                                                             \
              else                                                            \
                {                                                             \
                  if (!(_eva_rb_del_w->left && is_red (_eva_rb_del_w->left))) \
                    {                                                         \
                      set_is_red (_eva_rb_del_w->right, 0);                   \
                      set_is_red (_eva_rb_del_w, 1);                          \
                      EVA_RBTREE_ROTATE_LEFT (top,type,parent,left,right,     \
                                              _eva_rb_del_w);                 \
                      _eva_rb_del_w = _eva_rb_del_xparent->left;              \
                    }                                                         \
                  set_is_red (_eva_rb_del_w, is_red (_eva_rb_del_xparent));   \
                  set_is_red (_eva_rb_del_xparent, 0);                        \
                  if (_eva_rb_del_w->left)                                    \
                    set_is_red (_eva_rb_del_w->left, 0);                      \
                  EVA_RBTREE_ROTATE_RIGHT (top,type,parent,left,right,        \
                                           _eva_rb_del_xparent);              \
                  _eva_rb_del_x = top;                                        \
                }                                                             \
            }                                                                 \
        }                                                                     \
      if (_eva_rb_del_x != NULL)                                              \
        set_is_red(_eva_rb_del_x, 0);                                         \
    }                                                                         \
  _eva_rb_del_z->left = NULL;                                                 \
  _eva_rb_del_z->right = NULL;                                                \
  _eva_rb_del_z->parent = NULL;                                               \
}G_STMT_END

#define EVA_RBTREE_LOOKUP_COMPARATOR_(top,type,is_red,set_is_red,parent,left,right,comparator, \
                                      key,key_comparator,out)                 \
G_STMT_START{                                                                 \
  type _eva_lookup_at = (top);                                                \
  while (_eva_lookup_at)                                                      \
    {                                                                         \
      int _eva_compare_rv;                                                    \
      key_comparator(key,_eva_lookup_at,_eva_compare_rv);                     \
      if (_eva_compare_rv < 0)                                                \
        _eva_lookup_at = _eva_lookup_at->left;                                \
      else if (_eva_compare_rv > 0)                                           \
        _eva_lookup_at = _eva_lookup_at->right;                               \
      else                                                                    \
        break;                                                                \
    }                                                                         \
  out = _eva_lookup_at;                                                       \
}G_STMT_END
 /* see comments for 'SUPREMUM'; it is the same with the sense of the comparators
  * and left,right reversed. */
#define EVA_RBTREE_INFIMUM_COMPARATOR_(top,type,is_red,set_is_red,parent,left,right,comparator, \
                                      key,key_comparator,out)                 \
G_STMT_START{                                                                 \
  type _eva_lookup_at = (top);                                                \
  type _eva_lookup_rv = NULL;                                                 \
  while (_eva_lookup_at)                                                      \
    {                                                                         \
      int _eva_compare_rv;                                                    \
      key_comparator(key,_eva_lookup_at,_eva_compare_rv);                     \
      if (_eva_compare_rv >= 0)                                               \
        {                                                                     \
          _eva_lookup_rv = _eva_lookup_at;                                    \
          _eva_lookup_at = _eva_lookup_at->right;                             \
        }                                                                     \
      else                                                                    \
        _eva_lookup_at = _eva_lookup_at->left;                                \
    }                                                                         \
  out = _eva_lookup_rv;                                                       \
}G_STMT_END
/* see introductory comments for a less mathematical
 * definition.  but what 'supremum' computes is:
 * sup(tree, key) = min S(tree,key) or NULL if S(tree, key)
 * is empty, where S(tree,key) = { t | t \in tree && t >= key }.
 * The 'min' is determined by the 'comparator',
 * and the 't >= key' is determined by the 'key_comparator'.
 * But they must be consistent.
 *
 * so, here's a recursive description.  suppose we wish to compute
 * the supremum sup(a, key), where 'a' is the node in the tree shown:
 *                    a       
 *                   / \      
 *                  b   c     
 * Is 'a >= key'?  Then 'a' is in S(a, key),                
 * and hence sup(a,key) exists.  But a "better" supremum,   
 * in terms of being the 'min' in the tree,                 
 * may lie in 'b'. Nothing better may lie in 'c', since it
 * is larger, and we are computing a minimum of S.
 * 
 * But if 'a < key', then 'a' is not in S.  hence 'b' and its children
 * are not in S.  Hence sup(a) = sup(c), including the possibility that
 * sup(C) = NULL.
 *
 * Therefore,
 *    
 *              sup(b)     if 'a >= key', and sub(b) exists,         [case0]
 *     sup(a) = a          if 'a >= key', and sub(b) does not exist, [case1]
 *              sup(c)     if 'a < key' and sub(c) exists,           [case2]
 *              NULL       if 'a < key' and sub(c) does not exist.   [case3]
 *
 * the non-recursive algo follows once you realize that it's just
 * a tree descent, keeping track of the best candidate you've found.
 * TODO: there's got to be a better way to describe it.
 */
#define EVA_RBTREE_SUPREMUM_COMPARATOR_(top,type,is_red,set_is_red,parent,left,right,comparator, \
                                      key,key_comparator,out)                 \
G_STMT_START{                                                                 \
  type _eva_lookup_at = (top);                                                \
  type _eva_lookup_rv = NULL;                                                 \
  while (_eva_lookup_at)                                                      \
    {                                                                         \
      int _eva_compare_rv;                                                    \
      key_comparator(key,_eva_lookup_at,_eva_compare_rv);                     \
      if (_eva_compare_rv <= 0)                                               \
        {                                                                     \
          _eva_lookup_rv = _eva_lookup_at;                                    \
          _eva_lookup_at = _eva_lookup_at->left;                              \
        }                                                                     \
      else                                                                    \
        _eva_lookup_at = _eva_lookup_at->right;                               \
    }                                                                         \
  out = _eva_lookup_rv;                                                       \
}G_STMT_END
#define EVA_RBTREE_LOOKUP_(top,type,is_red,set_is_red,parent,left,right,comparator, key,out) \
  EVA_RBTREE_LOOKUP_COMPARATOR_(top,type,is_red,set_is_red,parent,left,right,comparator, key,comparator,out)
#define EVA_RBTREE_SUPREMUM_(top,type,is_red,set_is_red,parent,left,right,comparator, key,out) \
  EVA_RBTREE_SUPREMUM_COMPARATOR_(top,type,is_red,set_is_red,parent,left,right,comparator, key,comparator,out)
#define EVA_RBTREE_INFIMUM_(top,type,is_red,set_is_red,parent,left,right,comparator, key,out) \
  EVA_RBTREE_INFIMUM_COMPARATOR_(top,type,is_red,set_is_red,parent,left,right,comparator, key,comparator,out)

/* these macros don't need the is_red/set_is_red macros, nor the comparator,
   so omit them, to keep the lines a bit shorter. */
#define EVA_RBTREE_ROTATE_RIGHT(top,type,parent,left,right, node)             \
  EVA_RBTREE_ROTATE_LEFT(top,type,parent,right,left, node)
#define EVA_RBTREE_ROTATE_LEFT(top,type,parent,left,right, node)              \
G_STMT_START{                                                                 \
  type _eva_rot_x = (node);                                                   \
  type _eva_rot_y = _eva_rot_x->right;                                        \
                                                                              \
  _eva_rot_x->right = _eva_rot_y->left;                                       \
  if (_eva_rot_y->left)                                                       \
    _eva_rot_y->left->parent = _eva_rot_x;                                    \
  _eva_rot_y->parent = _eva_rot_x->parent;                                    \
  if (_eva_rot_x->parent == NULL)                                             \
    top = _eva_rot_y;                                                         \
  else if (_eva_rot_x == _eva_rot_x->parent->left)                            \
    _eva_rot_x->parent->left = _eva_rot_y;                                    \
  else                                                                        \
    _eva_rot_x->parent->right = _eva_rot_y;                                   \
  _eva_rot_y->left = _eva_rot_x;                                              \
  _eva_rot_x->parent = _eva_rot_y;                                            \
}G_STMT_END

/* iteration */
#define EVA_RBTREE_NEXT_(top,type,is_red,set_is_red,parent,left,right,comparator, in, out)  \
G_STMT_START{                                                                 \
  type _eva_next_at = (in);                                                   \
  g_assert (_eva_next_at != NULL);                                            \
  if (_eva_next_at->right != NULL)                                            \
    {                                                                         \
      _eva_next_at = _eva_next_at->right;                                     \
      while (_eva_next_at->left != NULL)                                      \
        _eva_next_at = _eva_next_at->left;                                    \
      out = _eva_next_at;                                                     \
    }                                                                         \
  else                                                                        \
    {                                                                         \
      type _eva_next_parent = (in)->parent;                                   \
      while (_eva_next_parent != NULL                                         \
          && _eva_next_at == _eva_next_parent->right)                         \
        {                                                                     \
          _eva_next_at = _eva_next_parent;                                    \
          _eva_next_parent = _eva_next_parent->parent;                        \
        }                                                                     \
      out = _eva_next_parent;                                                 \
    }                                                                         \
}G_STMT_END

/* prev is just next with left/right child reversed. */
#define EVA_RBTREE_PREV_(top,type,is_red,set_is_red,parent,left,right,comparator, in, out)  \
  EVA_RBTREE_NEXT_(top,type,is_red,set_is_red,parent,right,left,comparator, in, out)

#define EVA_RBTREE_FIRST_(top,type,is_red,set_is_red,parent,left,right,comparator, out)  \
G_STMT_START{                                                                 \
  type _eva_first_at = (top);                                                 \
  if (_eva_first_at != NULL)                                                  \
    while (_eva_first_at->left != NULL)                                       \
      _eva_first_at = _eva_first_at->left;                                    \
  out = _eva_first_at;                                                        \
}G_STMT_END
#define EVA_RBTREE_LAST_(top,type,is_red,set_is_red,parent,left,right,comparator, out)  \
  EVA_RBTREE_FIRST_(top,type,is_red,set_is_red,parent,right,left,comparator, out)

 /* --- RBC-Tree --- */
#define EVA_RBCTREE_INSERT(tree, node, collision_node)                         \
  EVA_RBCTREE_INSERT_(tree, node, collision_node)
#define EVA_RBCTREE_REMOVE(tree, node)                                         \
  EVA_RBCTREE_REMOVE_(tree, node)
#define EVA_RBCTREE_LOOKUP(tree, key, out)                                     \
  EVA_RBCTREE_LOOKUP_(tree, key, out)
#define EVA_RBCTREE_LOOKUP_COMPARATOR(tree, key, key_comparator, out)          \
  EVA_RBCTREE_LOOKUP_COMPARATOR_(tree, key, key_comparator, out)

#define EVA_RBCTREE_FIRST(tree, out)                                           \
  EVA_RBCTREE_FIRST_(tree, out)
#define EVA_RBCTREE_LAST(tree, out)                                            \
  EVA_RBCTREE_LAST_(tree, out)
#define EVA_RBCTREE_NEXT(tree, in, out)                                        \
  EVA_RBCTREE_NEXT_(tree, in, out)
#define EVA_RBCTREE_PREV(tree, in, out)                                        \
  EVA_RBCTREE_PREV_(tree, in, out)

#define EVA_RBCTREE_SUPREMUM(tree, key, out)                                   \
  EVA_RBCTREE_SUPREMUM_(tree, key, out)
#define EVA_RBCTREE_SUPREMUM_COMPARATOR(tree, key, key_comparator, out)        \
  EVA_RBCTREE_SUPREMUM_COMPARATOR_(tree, key, key_comparator, out)
#define EVA_RBCTREE_INFIMUM(tree, key, out)                                    \
  EVA_RBCTREE_INFIMUM_(tree, key, out)
#define EVA_RBCTREE_INFIMUM_COMPARATOR(tree, key, key_comparator, out)         \
  EVA_RBCTREE_INFIMUM_COMPARATOR_(tree, key, key_comparator, out)

#define EVA_RBCTREE_GET_BY_INDEX(tree, index, out)                             \
  EVA_RBCTREE_GET_BY_INDEX_(tree, index, out)
#define EVA_RBCTREE_GET_BY_INDEX_UNCHECKED(tree, index, out)                   \
  EVA_RBCTREE_GET_BY_INDEX_UNCHECKED_(tree, index, out)
#define EVA_RBCTREE_GET_NODE_INDEX(tree, node, index_out)                      \
  EVA_RBCTREE_GET_NODE_INDEX_(tree, node, index_out)

#if 1
#undef G_STMT_START
#define G_STMT_START do
#undef G_STMT_END
#define G_STMT_END while(0)
#endif


#define EVA_RBCTREE_INSERT_(top,type,is_red,set_is_red,get_count,set_count,parent,left,right,comparator, node,collision_node) \
G_STMT_START{                                                                 \
  type _eva_last = NULL;                                                      \
  type _eva_at = (top);                                                       \
  gboolean _eva_last_was_left = FALSE;                                        \
  collision_node = NULL;                                                      \
  while (_eva_at != NULL)                                                     \
    {                                                                         \
      int _eva_compare_rv;                                                    \
      _eva_last = _eva_at;                                                    \
      comparator(_eva_at, (node), _eva_compare_rv);                           \
      if (_eva_compare_rv > 0)                                                \
        {                                                                     \
          _eva_last_was_left = TRUE;                                          \
          _eva_at = _eva_at->left;                                            \
        }                                                                     \
      else if (_eva_compare_rv < 0)                                           \
        {                                                                     \
          _eva_last_was_left = FALSE;                                         \
          _eva_at = _eva_at->right;                                           \
        }                                                                     \
      else                                                                    \
        break;                                                                \
   }                                                                          \
  if (_eva_at != NULL)                                                        \
    {                                                                         \
      /* collision */                                                         \
      collision_node = _eva_at;                                               \
    }                                                                         \
  else if (_eva_last == NULL)                                                 \
    {                                                                         \
      /* only node in tree */                                                 \
      top = (node);                                                           \
      set_is_red ((node), 0);                                                 \
      (node)->left = (node)->right = (node)->parent = NULL;                   \
      set_count ((node), 1);                                                  \
    }                                                                         \
  else                                                                        \
    {                                                                         \
      (node)->parent = _eva_last;                                             \
      (node)->left = (node)->right = NULL;                                    \
      if (_eva_last_was_left)                                                 \
        _eva_last->left = (node);                                             \
      else                                                                    \
        _eva_last->right = (node);                                            \
                                                                              \
      /* fixup counts */                                                      \
      set_count ((node), 1);                                                  \
      for (_eva_at = _eva_last; _eva_at; _eva_at = _eva_at->parent)           \
        {                                                                     \
          guint _eva_new_count = get_count(_eva_at) + 1;                      \
          set_count(_eva_at, _eva_new_count);                                 \
        }                                                                     \
                                                                              \
      /* fixup */                                                             \
      _eva_at = (node);                                                       \
      set_is_red (_eva_at, 1);                                                \
      while (_eva_at->parent != NULL && is_red(_eva_at->parent))              \
        {                                                                     \
          if (_eva_at->parent == _eva_at->parent->parent->left)               \
            {                                                                 \
              type _eva_y = _eva_at->parent->parent->right;                   \
              if (_eva_y != NULL && is_red (_eva_y))                          \
                {                                                             \
                  set_is_red (_eva_at->parent, 0);                            \
                  set_is_red (_eva_y, 0);                                     \
                  set_is_red (_eva_at->parent->parent, 1);                    \
                  _eva_at = _eva_at->parent->parent;                          \
                }                                                             \
              else                                                            \
                {                                                             \
                  if (_eva_at == _eva_at->parent->right)                      \
                    {                                                         \
                      _eva_at = _eva_at->parent;                              \
                      EVA_RBCTREE_ROTATE_LEFT (top,type,parent,left,right,get_count,set_count, _eva_at);\
                    }                                                         \
                  set_is_red(_eva_at->parent, 0);                             \
                  set_is_red(_eva_at->parent->parent, 1);                     \
                  EVA_RBCTREE_ROTATE_RIGHT (top,type,parent,left,right,get_count,set_count,\
                                         _eva_at->parent->parent);            \
                }                                                             \
            }                                                                 \
          else                                                                \
            {                                                                 \
              type _eva_y = _eva_at->parent->parent->left;                    \
              if (_eva_y != NULL && is_red (_eva_y))                          \
                {                                                             \
                  set_is_red (_eva_at->parent, 0);                            \
                  set_is_red (_eva_y, 0);                                     \
                  set_is_red (_eva_at->parent->parent, 1);                    \
                  _eva_at = _eva_at->parent->parent;                          \
                }                                                             \
              else                                                            \
                {                                                             \
                  if (_eva_at == _eva_at->parent->left)                       \
                    {                                                         \
                      _eva_at = _eva_at->parent;                              \
                      EVA_RBCTREE_ROTATE_RIGHT (top,type,parent,left,right,get_count,set_count,\
                                             _eva_at);                        \
                    }                                                         \
                  set_is_red(_eva_at->parent, 0);                             \
                  set_is_red(_eva_at->parent->parent, 1);                     \
                  EVA_RBCTREE_ROTATE_LEFT (top,type,parent,left,right,get_count,set_count,\
                                          _eva_at->parent->parent);           \
                }                                                             \
            }                                                                 \
        }                                                                     \
      set_is_red((top), 0);                                                   \
    }                                                                         \
}G_STMT_END

#define EVA_RBCTREE_REMOVE_(top,type,is_red,set_is_red,get_count,set_count,parent,left,right,comparator, node) \
/* Algorithms:273. */                                                         \
G_STMT_START{                                                                 \
  type _eva_rb_del_z = (node);                                                \
  type _eva_rb_del_x;                                                         \
  type _eva_rb_del_y;                                                         \
  type _eva_rb_del_nullpar = NULL;	/* Used to emulate sentinel nodes */  \
  gboolean _eva_rb_del_fixup;                                                 \
  if (_eva_rb_del_z->left == NULL || _eva_rb_del_z->right == NULL)            \
    _eva_rb_del_y = _eva_rb_del_z;                                            \
  else                                                                        \
    {                                                                         \
      EVA_RBTREE_NEXT_ (top,type,is_red,set_is_red,parent,left,right,comparator,\
                        _eva_rb_del_z, _eva_rb_del_y);                        \
    }                                                                         \
  _eva_rb_del_x = _eva_rb_del_y->left ? _eva_rb_del_y->left                   \
                                            : _eva_rb_del_y->right;           \
  if (_eva_rb_del_x)                                                          \
    _eva_rb_del_x->parent = _eva_rb_del_y->parent;                            \
  else                                                                        \
    _eva_rb_del_nullpar = _eva_rb_del_y->parent;                              \
  if (!_eva_rb_del_y->parent)                                                 \
    top = _eva_rb_del_x;                                                      \
  else                                                                        \
    {                                                                         \
      if (_eva_rb_del_y == _eva_rb_del_y->parent->left)                       \
	_eva_rb_del_y->parent->left = _eva_rb_del_x;                          \
      else                                                                    \
	_eva_rb_del_y->parent->right = _eva_rb_del_x;                         \
      _EVA_RBCTREE_FIX_COUNT_AND_UP(type,parent,left,right,get_count,set_count, _eva_rb_del_y->parent);\
    }                                                                         \
  _eva_rb_del_fixup = !is_red(_eva_rb_del_y);                                 \
  if (_eva_rb_del_y != _eva_rb_del_z)                                         \
    {                                                                         \
      set_is_red(_eva_rb_del_y, is_red(_eva_rb_del_z));                       \
      _eva_rb_del_y->left = _eva_rb_del_z->left;                              \
      _eva_rb_del_y->right = _eva_rb_del_z->right;                            \
      _eva_rb_del_y->parent = _eva_rb_del_z->parent;                          \
      if (_eva_rb_del_y->parent)                                              \
	{                                                                     \
	  if (_eva_rb_del_y->parent->left == _eva_rb_del_z)                   \
	    _eva_rb_del_y->parent->left = _eva_rb_del_y;                      \
	  else                                                                \
	    _eva_rb_del_y->parent->right = _eva_rb_del_y;                     \
	}                                                                     \
      else                                                                    \
        {                                                                     \
          top = _eva_rb_del_y;                                                \
        }                                                                     \
      /* TODO: look at pictures to see if "_AND_UP" is necessary */           \
      _EVA_RBCTREE_FIX_COUNT_AND_UP(type,parent,left,right,get_count,set_count, _eva_rb_del_y);\
                                                                              \
      if (_eva_rb_del_y->left)                                                \
	_eva_rb_del_y->left->parent = _eva_rb_del_y;                          \
      if (_eva_rb_del_y->right)                                               \
	_eva_rb_del_y->right->parent = _eva_rb_del_y;                         \
      if (_eva_rb_del_nullpar == _eva_rb_del_z)                               \
	_eva_rb_del_nullpar = _eva_rb_del_y;                                  \
    }                                                                         \
  if (_eva_rb_del_fixup)                                                      \
    {                                                                         \
      /* delete fixup (Algorithms, p 274) */                                  \
      while (_eva_rb_del_x != top                                             \
         && !(_eva_rb_del_x != NULL && is_red (_eva_rb_del_x)))               \
        {                                                                     \
          type _eva_rb_del_xparent = _eva_rb_del_x ? _eva_rb_del_x->parent    \
                                                   : _eva_rb_del_nullpar;     \
          if (_eva_rb_del_x == _eva_rb_del_xparent->left)                     \
            {                                                                 \
              type _eva_rb_del_w = _eva_rb_del_xparent->right;                \
              if (_eva_rb_del_w != NULL && is_red (_eva_rb_del_w))            \
                {                                                             \
                  set_is_red (_eva_rb_del_w, 0);                              \
                  set_is_red (_eva_rb_del_xparent, 1);                        \
                  EVA_RBCTREE_ROTATE_LEFT (top,type,parent,left,right,get_count,set_count, \
                                          _eva_rb_del_xparent);               \
                  _eva_rb_del_w = _eva_rb_del_xparent->right;                 \
                }                                                             \
              if (!(_eva_rb_del_w->left && is_red (_eva_rb_del_w->left))      \
               && !(_eva_rb_del_w->right && is_red (_eva_rb_del_w->right)))   \
                {                                                             \
                  set_is_red (_eva_rb_del_w, 1);                              \
                  _eva_rb_del_x = _eva_rb_del_xparent;                        \
                }                                                             \
              else                                                            \
                {                                                             \
                  if (!(_eva_rb_del_w->right && is_red (_eva_rb_del_w->right)))\
                    {                                                         \
                      if (_eva_rb_del_w->left)                                \
                        set_is_red (_eva_rb_del_w->left, 0);                  \
                      set_is_red (_eva_rb_del_w, 1);                          \
                      EVA_RBCTREE_ROTATE_RIGHT (top,type,parent,left,right,get_count,set_count, \
                                               _eva_rb_del_w);                \
                      _eva_rb_del_w = _eva_rb_del_xparent->right;             \
                    }                                                         \
                  set_is_red (_eva_rb_del_w, is_red (_eva_rb_del_xparent));   \
                  set_is_red (_eva_rb_del_xparent, 0);                        \
                  set_is_red (_eva_rb_del_w->right, 0);                       \
                  EVA_RBCTREE_ROTATE_LEFT (top,type,parent,left,right,get_count,set_count, \
                                          _eva_rb_del_xparent);               \
                  _eva_rb_del_x = top;                                        \
                }                                                             \
            }                                                                 \
          else                                                                \
            {                                                                 \
              type _eva_rb_del_w = _eva_rb_del_xparent->left;                 \
              if (_eva_rb_del_w && is_red (_eva_rb_del_w))                    \
                {                                                             \
                  set_is_red (_eva_rb_del_w, 0);                              \
                  set_is_red (_eva_rb_del_xparent, 1);                        \
                  EVA_RBCTREE_ROTATE_RIGHT (top,type,parent,left,right,get_count,set_count, \
                                           _eva_rb_del_xparent);              \
                  _eva_rb_del_w = _eva_rb_del_xparent->left;                  \
                }                                                             \
              if (!(_eva_rb_del_w->right && is_red (_eva_rb_del_w->right))    \
               && !(_eva_rb_del_w->left && is_red (_eva_rb_del_w->left)))     \
                {                                                             \
                  set_is_red (_eva_rb_del_w, 1);                              \
                  _eva_rb_del_x = _eva_rb_del_xparent;                        \
                }                                                             \
              else                                                            \
                {                                                             \
                  if (!(_eva_rb_del_w->left && is_red (_eva_rb_del_w->left))) \
                    {                                                         \
                      set_is_red (_eva_rb_del_w->right, 0);                   \
                      set_is_red (_eva_rb_del_w, 1);                          \
                      EVA_RBCTREE_ROTATE_LEFT (top,type,parent,left,right,get_count,set_count, \
                                              _eva_rb_del_w);                 \
                      _eva_rb_del_w = _eva_rb_del_xparent->left;              \
                    }                                                         \
                  set_is_red (_eva_rb_del_w, is_red (_eva_rb_del_xparent));   \
                  set_is_red (_eva_rb_del_xparent, 0);                        \
                  if (_eva_rb_del_w->left)                                    \
                    set_is_red (_eva_rb_del_w->left, 0);                      \
                  EVA_RBCTREE_ROTATE_RIGHT (top,type,parent,left,right,get_count,set_count, \
                                           _eva_rb_del_xparent);              \
                  _eva_rb_del_x = top;                                        \
                }                                                             \
            }                                                                 \
        }                                                                     \
      if (_eva_rb_del_x != NULL)                                              \
        set_is_red(_eva_rb_del_x, 0);                                         \
    }                                                                         \
  _eva_rb_del_z->left = NULL;                                                 \
  _eva_rb_del_z->right = NULL;                                                \
  _eva_rb_del_z->parent = NULL;                                               \
}G_STMT_END

#define EVA_RBCTREE_LOOKUP_COMPARATOR_(top,type,is_red,set_is_red,get_count,set_count,parent,left,right,comparator, \
                                      key,key_comparator,out)                 \
 EVA_RBTREE_LOOKUP_COMPARATOR_(top,type,is_red,set_count,parent,left,right,comparator, key,key_comparator,out)
#define EVA_RBCTREE_INFIMUM_COMPARATOR_(top,type,is_red,set_is_red,get_count,set_count,parent,left,right,comparator, \
                                      key,key_comparator,out)                 \
 EVA_RBTREE_INFIMUM_COMPARATOR_(top,type,is_red,set_is_red,parent,left,right,comparator, key,key_comparator,out)
#define EVA_RBCTREE_SUPREMUM_COMPARATOR_(top,type,is_red,set_is_red,get_count,set_count,parent,left,right,comparator, \
                                      key,key_comparator,out)                 \
 EVA_RBTREE_SUPREMUM_COMPARATOR_(top,type,is_red,set_is_red,parent,left,right,comparator, key,key_comparator,out)
#define EVA_RBCTREE_LOOKUP_(top,type,is_red,set_is_red,get_count,set_count,parent,left,right,comparator, key,out) \
  EVA_RBTREE_LOOKUP_COMPARATOR_(top,type,is_red,set_is_red,parent,left,right,comparator, key,comparator,out)
#define EVA_RBCTREE_SUPREMUM_(top,type,is_red,set_is_red,get_count,set_count,parent,left,right,comparator, key,out) \
  EVA_RBTREE_SUPREMUM_COMPARATOR_(top,type,is_red,set_is_red,parent,left,right,comparator, key,comparator,out)
#define EVA_RBCTREE_INFIMUM_(top,type,is_red,set_is_red,get_count,set_count,parent,left,right,comparator, key,out) \
  EVA_RBTREE_INFIMUM_COMPARATOR_(top,type,is_red,set_is_red,parent,left,right,comparator, key,comparator,out)
#define EVA_RBCTREE_NEXT_(top,type,is_red,set_is_red,get_count,set_count,parent,left,right,comparator, in, out)  \
   EVA_RBTREE_NEXT_(top,type,is_red,set_is_red,parent,left,right,comparator, in, out)
#define EVA_RBCTREE_PREV_(top,type,is_red,set_is_red,parent,left,right,comparator, in, out)  \
   EVA_RBTREE_PREV_(top,type,is_red,set_is_red,parent,left,right,comparator, in, out)
#define EVA_RBCTREE_FIRST_(top,type,is_red,set_is_red,get_count,set_count,parent,left,right,comparator, out)  \
  EVA_RBTREE_FIRST_(top,type,is_red,set_is_red,parent,left,right,comparator, out)
#define EVA_RBCTREE_LAST_(top,type,is_red,set_is_red,get_count,set_count,parent,left,right,comparator, out)  \
  EVA_RBTREE_LAST_(top,type,is_red,set_is_red,parent,left,right,comparator, out)

#define EVA_RBCTREE_GET_BY_INDEX_(top,type,is_red,set_is_red,get_count,set_count,parent,left,right,comparator, index,out) \
  G_STMT_START{                                                                \
    if (top == NULL || (index) >= get_count(top))                              \
      out = NULL;                                                              \
    else                                                                       \
      EVA_RBCTREE_GET_BY_INDEX_UNCHECKED_(top,type,is_red,set_is_red,get_count,set_count,parent,left,right,comparator, index,out);\
  }G_STMT_END
#define EVA_RBCTREE_GET_BY_INDEX_UNCHECKED_(top,type,is_red,set_is_red,get_count,set_count,parent,left,right,comparator, index,out) \
   G_STMT_START{                                                               \
     type _eva_at = (top);                                                     \
     guint _eva_index = (index);                                               \
     for (;;)                                                                  \
       {                                                                       \
         guint _eva_left_size = _eva_at->left ? get_count(_eva_at->left) : 0;  \
         if (_eva_index < _eva_left_size)                                      \
           _eva_at = _eva_at->left;                                            \
         else if (_eva_index == _eva_left_size)                                \
           break;                                                              \
         else                                                                  \
           {                                                                   \
             _eva_index -= (_eva_left_size + 1);                               \
             _eva_at = _eva_at->right;                                         \
           }                                                                   \
       }                                                                       \
     out = _eva_at;                                                            \
   }G_STMT_END

#define EVA_RBCTREE_GET_NODE_INDEX_(top,type,is_red,set_is_red,get_count,set_count,parent,left,right,comparator, node,index_out) \
   G_STMT_START{                                                               \
     type _eva_at = (node);                                                    \
     guint _eva_rv = _eva_at->left ? get_count (_eva_at->left) : 0;            \
     while (_eva_at->parent != NULL)                                           \
       {                                                                       \
         if (_eva_at->parent->left == _eva_at)                                 \
           ;                                                                   \
         else                                                                  \
           {                                                                   \
             if (_eva_at->parent->left)                                        \
               _eva_rv += get_count((_eva_at->parent->left));                  \
             _eva_rv += 1;                                                     \
           }                                                                   \
         _eva_at = _eva_at->parent;                                            \
       }                                                                       \
     index_out = _eva_rv;                                                      \
   }G_STMT_END

#define EVA_RBCTREE_ROTATE_RIGHT(top,type,parent,left,right,get_count,set_count, node) \
  EVA_RBCTREE_ROTATE_LEFT(top,type,parent,right,left,get_count,set_count, node)
#define EVA_RBCTREE_ROTATE_LEFT(top,type,parent,left,right,get_count,set_count, node)  \
G_STMT_START{                                                                 \
  type _eva_rot_x = (node);                                                   \
  type _eva_rot_y = _eva_rot_x->right;                                        \
                                                                              \
  _eva_rot_x->right = _eva_rot_y->left;                                       \
  if (_eva_rot_y->left)                                                       \
    _eva_rot_y->left->parent = _eva_rot_x;                                    \
  _eva_rot_y->parent = _eva_rot_x->parent;                                    \
  if (_eva_rot_x->parent == NULL)                                             \
    top = _eva_rot_y;                                                         \
  else if (_eva_rot_x == _eva_rot_x->parent->left)                            \
    _eva_rot_x->parent->left = _eva_rot_y;                                    \
  else                                                                        \
    _eva_rot_x->parent->right = _eva_rot_y;                                   \
  _eva_rot_y->left = _eva_rot_x;                                              \
  _eva_rot_x->parent = _eva_rot_y;                                            \
                                                                              \
  /* fixup counts. */                                                         \
  /* to re-derive this here's what rotate_left(x) does pictorially: */        \
  /*       x                                 y                      */        \
  /*      / \                               / \                     */        \
  /*     a   y     == rotate_left(x) ==>   x   c                    */        \
  /*        / \                           / \                       */        \
  /*       b   c                         a   b                      */        \
  /*                                                                */        \
  /* so:       n0 = get_count(x) (==a+b+c+2)                        */        \
  /*           n1 = get_count(c)   (c may be null)                  */        \
  /*           n2 = n0 - n1 - 1;                                    */        \
  /*           set_count(x, n2)                                     */        \
  /*           set_count(y, n0)                                     */        \
  /*     c is _eva_rot_y->right at this point                       */        \
  /*                                                                */        \
  /* equivalently:                                                  */        \
  /*      y->count = x->count;                                      */        \
  /*      x->count -= c->count + 1                                  */        \
  {                                                                           \
    guint _eva_rot_n0 = get_count(_eva_rot_x);                                \
    guint _eva_rot_n1 = _eva_rot_y->right ? get_count(_eva_rot_y->right) : 0; \
    guint _eva_rot_n2 = _eva_rot_n0 - _eva_rot_n1 - 1;                        \
    set_count (_eva_rot_x, _eva_rot_n2);                                      \
    set_count (_eva_rot_y, _eva_rot_n0);                                      \
  }                                                                           \
}G_STMT_END

 /* utility: recompute node's count, based on count of its children */
#define _EVA_RBCTREE_FIX_COUNT(type,parent,left,right,get_count,set_count, node)          \
G_STMT_START{                                                                 \
  guint _eva_fixcount_count = 1;                                              \
  if ((node)->left != NULL)                                                   \
    _eva_fixcount_count += get_count((node)->left);                           \
  if ((node)->right != NULL)                                                  \
    _eva_fixcount_count += get_count((node)->right);                          \
  set_count((node), _eva_fixcount_count);                                     \
}G_STMT_END

 /* utility: recompute node's count, based on count of its children */
#define _EVA_RBCTREE_FIX_COUNT_AND_UP(type,parent,left,right,get_count,set_count, node)   \
G_STMT_START{                                                                 \
  type _tmp_fix_count_up;                                                     \
  for (_tmp_fix_count_up = (node);                                            \
       _tmp_fix_count_up != NULL;                                             \
       _tmp_fix_count_up = _tmp_fix_count_up->parent)                         \
    _EVA_RBCTREE_FIX_COUNT (type,parent,left,right,get_count,set_count, _tmp_fix_count_up);\
}G_STMT_END

#endif
