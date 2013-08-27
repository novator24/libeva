#ifndef __EVA_MEM_POOL_H_
#define __EVA_MEM_POOL_H_

#include <glib.h>

G_BEGIN_DECLS

typedef struct _EvaMemPool EvaMemPool;
typedef struct _EvaMemPoolFixed EvaMemPoolFixed;

/* --- Allocate-only Memory Pool --- */
struct _EvaMemPool
{
  /*< private >*/
  gpointer all_chunk_list;
  char *chunk;
  guint chunk_left;
};

#define EVA_MEM_POOL_STATIC_INIT                        { NULL, NULL, 0 }


G_INLINE_FUNC void     eva_mem_pool_construct    (EvaMemPool     *pool);
G_INLINE_FUNC void     eva_mem_pool_construct_with_scratch_buf
                                                 (EvaMemPool     *pool,
                                                  gpointer        buffer,
                                                  gsize           buffer_size);
G_INLINE_FUNC gpointer eva_mem_pool_alloc        (EvaMemPool     *pool,
                                                  gsize           size);
              gpointer eva_mem_pool_alloc0       (EvaMemPool     *pool,
                                                  gsize           size);
G_INLINE_FUNC gpointer eva_mem_pool_alloc_unaligned(EvaMemPool   *pool,
                                                  gsize           size);
              char    *eva_mem_pool_strdup       (EvaMemPool     *pool,
                                                  const char     *str);
G_INLINE_FUNC void     eva_mem_pool_destruct     (EvaMemPool     *pool);

/* --- Allocate and free Memory Pool --- */
struct _EvaMemPoolFixed
{
  /*< private >*/
  gpointer slab_list;
  char *chunk;
  guint pieces_left;
  guint piece_size;
  gpointer free_list;
};

#define EVA_MEM_POOL_FIXED_STATIC_INIT(size) \
                          { NULL, NULL, 0, size, NULL } 

G_INLINE_FUNC void     eva_mem_pool_fixed_construct_with_scratch_buf
                                                 (EvaMemPoolFixed *pool,
                                                  gsize            elt_size,
                                                  gpointer         buffer,
                                                  gsize            buffer_n_elements);
void     eva_mem_pool_fixed_construct (EvaMemPoolFixed  *pool,
                                       gsize             size);
gpointer eva_mem_pool_fixed_alloc     (EvaMemPoolFixed  *pool);
gpointer eva_mem_pool_fixed_alloc0    (EvaMemPoolFixed  *pool);
void     eva_mem_pool_fixed_free      (EvaMemPoolFixed  *pool,
                                       gpointer          from_pool);
void     eva_mem_pool_fixed_destruct  (EvaMemPoolFixed  *pool);



/* private */
gpointer eva_mem_pool_must_alloc (EvaMemPool *pool,
                                  gsize       size);

/* ------------------------------*/
/* -- Inline Implementations --- */

#define _EVA_MEM_POOL_ALIGN(size)	\
  (((size) + sizeof(gpointer) - 1) / sizeof (gpointer) * sizeof (gpointer))
#define _EVA_MEM_POOL_SLAB_GET_NEXT_PTR(slab) \
  (* (gpointer*) (slab))

#if defined(G_CAN_INLINE) || defined(EVA_INTERNAL_IMPLEMENT_INLINES)
G_INLINE_FUNC void     eva_mem_pool_construct    (EvaMemPool     *pool)
{
  pool->all_chunk_list = NULL;
  pool->chunk = NULL;
  pool->chunk_left = 0;
}
G_INLINE_FUNC void     eva_mem_pool_construct_with_scratch_buf
                                                 (EvaMemPool     *pool,
                                                  gpointer        buffer,
                                                  gsize           buffer_size)
{
  pool->all_chunk_list = NULL;
  pool->chunk = buffer;
  pool->chunk_left = buffer_size;
}

G_INLINE_FUNC void     eva_mem_pool_align        (EvaMemPool     *pool)
{
  guint mask = GPOINTER_TO_UINT (pool->chunk) & (sizeof(gpointer)-1);
  if (mask)
    {
      /* need to align chunk */
      guint align = sizeof (gpointer) - mask;
      pool->chunk_left -= align;
      pool->chunk = (char*)pool->chunk + align;
    }
}

G_INLINE_FUNC gpointer eva_mem_pool_alloc_unaligned   (EvaMemPool     *pool,
                                                       gsize           size)
{
  char *rv;
  if (G_LIKELY (pool->chunk_left >= size))
    {
      rv = pool->chunk;
      pool->chunk_left -= size;
      pool->chunk = rv + size;
      return rv;
    }
  else
    /* fall through to non-inline version for
       slow malloc-using case */
    return eva_mem_pool_must_alloc (pool, size);
}

G_INLINE_FUNC gpointer eva_mem_pool_alloc            (EvaMemPool     *pool,
                                                      gsize           size)
{
  eva_mem_pool_align (pool);
  return eva_mem_pool_alloc_unaligned (pool, size);
}

G_INLINE_FUNC void     eva_mem_pool_destruct     (EvaMemPool     *pool)
{
  gpointer slab = pool->all_chunk_list;
  while (slab)
    {
      gpointer new_slab = _EVA_MEM_POOL_SLAB_GET_NEXT_PTR (slab);
      g_free (slab);
      slab = new_slab;
    }
}
G_INLINE_FUNC void     eva_mem_pool_fixed_construct_with_scratch_buf
                                                 (EvaMemPoolFixed *pool,
                                                  gsize            elt_size,
                                                  gpointer         buffer,
                                                  gsize            buffer_n_elements)
{
  pool->slab_list = NULL;
  pool->chunk = buffer;
  pool->pieces_left = buffer_n_elements;
  pool->piece_size = elt_size;
  pool->free_list = NULL;
}

#endif /* G_CAN_INLINE */

G_END_DECLS

#endif
