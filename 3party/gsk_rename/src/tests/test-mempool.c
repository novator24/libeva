#include "../evamempool.h"
#include "../evatree.h"

typedef struct _AllocInfo AllocInfo;
struct _AllocInfo
{
  char *allocation;
  guint size;
};

static gint
compare_by_pointer (gconstpointer a, gconstpointer b, gpointer udata)
{
  const AllocInfo *a_info = a;
  const AllocInfo *b_info = b;
  const char *aa = a_info->allocation;
  const char *bb = b_info->allocation;
  return (aa < bb) ? -1 : (aa > bb) ? 1 : 0;
}

static gboolean
alloc_infos_overlap (const AllocInfo *ai1, const AllocInfo *ai2)
{
  return ai1->allocation < ai2->allocation + ai2->size
      && ai2->allocation < ai1->allocation + ai1->size;
}

static void
record_allocation (EvaTree *tree, AllocInfo *info)
{
  EvaTreeNode *node;
  EvaTreeNode *next;
  EvaTreeNode *prev;
  g_assert (eva_tree_lookup (tree, info) == NULL);
  eva_tree_insert (tree, info, info);

  node = eva_tree_node_find (tree, info);
  g_assert (node != NULL);
  eva_tree_node_visit (tree, node);
  prev = eva_tree_node_prev (tree, node);
  if (prev)
    {
      g_assert (!alloc_infos_overlap (eva_tree_node_peek_key (prev), info));
      eva_tree_node_unvisit (tree, prev);
    }
  eva_tree_node_visit (tree, node);
  next = eva_tree_node_next (tree, node);
  if (next)
    {
      g_assert (!alloc_infos_overlap (eva_tree_node_peek_key (next), info));
      eva_tree_node_unvisit (tree, next);
    }
  eva_tree_node_unvisit (tree, node);
}

int main (int argc, char **argv)
{
  guint i;
  for (i = 0; i < 10; i++)
    {
      EvaMemPool pool;
      EvaTree *tree;
      guint j;
      tree = eva_tree_new_full (compare_by_pointer, NULL, g_free, NULL);
      eva_mem_pool_construct (&pool);
      for (j = 0; j < 1000; j++)
	{
	  AllocInfo *info = g_new (AllocInfo, 1);
	  info->size = g_random_int_range (10, 1000);
	  info->allocation = eva_mem_pool_alloc (&pool, info->size);
	  record_allocation (tree, info);
	}
      eva_tree_unref (tree);
      eva_mem_pool_destruct (&pool);
    }

  for (i = 4; i < 320; i += 15)
    {
      guint j;
      EvaTree *tree = eva_tree_new_full (compare_by_pointer, NULL, g_free, NULL);
      EvaMemPoolFixed pool;
      eva_mem_pool_fixed_construct (&pool, i);
      for (j = 0; j < 1000; j++)
	{
	  AllocInfo *info = g_new (AllocInfo, 1);
	  info->size = i;
	  info->allocation = eva_mem_pool_fixed_alloc (&pool);
	  record_allocation (tree, info);
	}
      eva_mem_pool_fixed_destruct (&pool);
      eva_tree_unref (tree);
    }
  return 0;
}
