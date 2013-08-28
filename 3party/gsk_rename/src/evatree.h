#ifndef __EVA_TREE_H_
#define __EVA_TREE_H_

#include <glib.h>

G_BEGIN_DECLS

typedef struct _EvaTreeNode EvaTreeNode;
typedef struct _EvaTree EvaTree;
EvaTree      *eva_tree_new             (GCompareFunc     compare);
EvaTree      *eva_tree_ref             (EvaTree         *tree);
void          eva_tree_unref           (EvaTree         *tree);
EvaTree      *eva_tree_new_full        (GCompareDataFunc compare,
                                        gpointer         compare_data,
                                        GDestroyNotify   key_destroy_func,
                                        GDestroyNotify   value_destroy_func);
void          eva_tree_insert          (EvaTree         *tree,
                                        gpointer         key,
                                        gpointer         value);
void          eva_tree_replace         (EvaTree         *tree,
                                        gpointer         key,
                                        gpointer         value);
gpointer      eva_tree_lookup          (EvaTree         *tree,
				        gpointer         key);
void          eva_tree_remove          (EvaTree         *tree,
					gpointer         key);
guint         eva_tree_n_nodes         (EvaTree         *tree);
                                       

/* iteration.
 *
 *  note that insert/remove/replace are allowed on any
 *  node during any number of iterations.  _prev/_next will
 *  never return a removed node but they will return nodes that
 *  were inserted after the iteration began.
 *
 *  When a node is returned to you it has a use_count that is
 *  incremented.  Functions that take a EvaTreeNode decrement
 *  that use count.  If you wish to abort iteration prematurely
 *  you should call eva_tree_node_unvisit(); if you wish to
 *  come back to a certain point you might use eva_tree_node_visit(),
 *  but that's rare.
 *
 *  Also note that we don't hold a reference to the tree!
 *  Hence make sure you eva_tree_ref/unref around iteration
 *  if you don't own the tree.
 */
EvaTreeNode  *eva_tree_node_first      (EvaTree         *tree);
EvaTreeNode  *eva_tree_node_last       (EvaTree         *tree);
EvaTreeNode  *eva_tree_node_find       (EvaTree         *tree,
					gpointer         search_key);
EvaTreeNode  *eva_tree_node_next       (EvaTree         *tree,
                                        EvaTreeNode     *node);
EvaTreeNode  *eva_tree_node_prev       (EvaTree         *tree,
                                        EvaTreeNode     *node);
gpointer      eva_tree_node_peek_key   (EvaTreeNode     *node);
gpointer      eva_tree_node_peek_value (EvaTreeNode     *node);
gboolean      eva_tree_node_is_removed (EvaTreeNode     *node);
void          eva_tree_node_visit      (EvaTree         *tree,
                                        EvaTreeNode     *node);
void          eva_tree_node_unvisit    (EvaTree         *tree,
                                        EvaTreeNode     *node);

gboolean      eva_tree_validate        (EvaTree         *tree);

G_END_DECLS

#endif
