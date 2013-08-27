
#include <glib.h>

typedef struct _GskQsortStackNode GskQsortStackNode;
typedef struct _GskQsortStack GskQsortStack;

/* Amount of stack to allocate: should be log2(max_array_size)+1.
   on 32-bit, this uses 33*8=264 bytes;
   on 64-bit it uses 65*16=1040 bytes. */
#if (GLIB_SIZEOF_SIZE_T == 8)
#define EVA_QSORT_STACK_MAX_SIZE  (65)
#elif (GLIB_SIZEOF_SIZE_T == 4)
#define EVA_QSORT_STACK_MAX_SIZE  (33)
#else
#error "sizeof(size_t) is neither 4 nor 8: need EVA_QSORT_STACK_MAX_SIZE def"
#endif

/* Maximum number of elements to sort with insertion sort instead of qsort */
#define EVA_INSERTION_SORT_THRESHOLD	4

struct _GskQsortStackNode
{
  gsize start, len;
};


#define EVA_QSORT(array, type, n_elements, compare)		             \
  EVA_QSORT_FULL(array, type, n_elements, compare,                           \
                 EVA_INSERTION_SORT_THRESHOLD,                               \
                 EVA_QSORT_STACK_MAX_SIZE,                                   \
                 /* no stack guard assertion */)

#define EVA_QSORT_DEBUG(array, type, n_elements, compare)		     \
  EVA_QSORT_FULL(array, type, n_elements, compare,                           \
                 EVA_INSERTION_SORT_THRESHOLD,                               \
                 EVA_QSORT_STACK_MAX_SIZE,                                   \
                 EVA_QSORT_ASSERT_STACK_SIZE)

#define EVA_QSELECT(array, type, n_elements, n_select, compare)		     \
  EVA_QSELECT_FULL(array, type, n_elements, n_select, compare,               \
                   EVA_INSERTION_SORT_THRESHOLD,                             \
                   EVA_QSORT_STACK_MAX_SIZE,                                 \
                   /* no stack guard assertion */)

#define EVA_QSORT_FULL(array, type, n_elements, compare, isort_threshold, stack_size, ss_assertion)    \
  G_STMT_START{                                                              \
    gint eva_rv;                                                             \
    guint eva_stack_size;                                                    \
    GskQsortStackNode eva_stack[stack_size];                                 \
    type eva_tmp_swap;                                                       \
    GskQsortStackNode eva_node;                                              \
    eva_node.start = 0;                                                      \
    eva_node.len = (n_elements);                                             \
    eva_stack_size = 0;                                                      \
    if (n_elements <= isort_threshold)                                       \
      EVA_INSERTION_SORT(array,type,n_elements,compare);                     \
    else                                                                     \
      for(;;)                                                                \
        {                                                                    \
          GskQsortStackNode eva_stack_nodes[2];                              \
          /* implement median-of-three; sort so that  */                     \
          /* *eva_a <= *eva_b <= *eva_c               */                     \
          type *eva_lo = array + eva_node.start;                             \
          type *eva_hi = eva_lo + eva_node.len - 1;                          \
          type *eva_a = eva_lo;                                              \
          type *eva_b = array + eva_node.start + eva_node.len / 2;           \
          type *eva_c = eva_hi;                                              \
          compare((*eva_a), (*eva_b), eva_rv);                               \
          if (eva_rv < 0)                                                    \
            {                                                                \
              compare((*eva_b), (*eva_c), eva_rv);                           \
              if (eva_rv <= 0)                                               \
                {                                                            \
                  /* a <= b <= c: already sorted */                          \
                }                                                            \
              else                                                           \
                {                                                            \
                  compare((*eva_a), (*eva_c), eva_rv);                       \
                  if (eva_rv <= 0)                                           \
                    {                                                        \
                      /* a <= c <= b */                                      \
                      eva_tmp_swap = *eva_b;                                 \
                      *eva_b = *eva_c;                                       \
                      *eva_c = eva_tmp_swap;                                 \
                    }                                                        \
                  else                                                       \
                    {                                                        \
                      /* c <= a <= b */                                      \
                      eva_tmp_swap = *eva_a;                                 \
                      *eva_a = *eva_c;                                       \
                      *eva_c = *eva_b;                                       \
                      *eva_b = eva_tmp_swap;                                 \
                    }                                                        \
                }                                                            \
            }                                                                \
          else                                                               \
            {                                                                \
              /* *b < *a */                                                  \
              compare((*eva_b), (*eva_c), eva_rv);                           \
              if (eva_rv >= 0)                                               \
                {                                                            \
                  /* *c <= *b < *a */                                        \
                  eva_tmp_swap = *eva_c;                                     \
                  *eva_c = *eva_a;                                           \
                  *eva_a = eva_tmp_swap;                                     \
                }                                                            \
              else                                                           \
                {                                                            \
                  /* b<a, b<c */                                             \
                  compare((*eva_a), (*eva_c), eva_rv);                       \
                  if (eva_rv >= 0)                                           \
                    {                                                        \
                      /* b < c <= a */                                       \
                      eva_tmp_swap = *eva_a;                                 \
                      *eva_a = *eva_b;                                       \
                      *eva_b = *eva_c;                                       \
                      *eva_c = eva_tmp_swap;                                 \
                    }                                                        \
                  else                                                       \
                    {                                                        \
                      /* b < a < c */                                        \
                      eva_tmp_swap = *eva_a;                                 \
                      *eva_a = *eva_b;                                       \
                      *eva_b = eva_tmp_swap;                                 \
                    }                                                        \
                }                                                            \
            }                                                                \
                                                                             \
          /* ok, phew, now *eva_a <= *eva_b <= *eva_c */                     \
                                                                             \
          /* partition this range of the array */                            \
          eva_a++;                                                           \
          eva_c--;                                                           \
          do                                                                 \
            {                                                                \
              /* advance eva_a to a element that violates */                 \
              /* partitioning (or it hits eva_b) */                          \
              for (;;)                                                       \
                {                                                            \
                  compare((*eva_a), (*eva_b), eva_rv);                       \
                  if (eva_rv >= 0)                                           \
                    break;                                                   \
                  eva_a++;                                                   \
                }                                                            \
              /* advance eva_c to a element that violates */                 \
              /* partitioning (or it hits eva_b) */                          \
              for (;;)                                                       \
                {                                                            \
                  compare((*eva_b), (*eva_c), eva_rv);                       \
                  if (eva_rv >= 0)                                           \
                    break;                                                   \
                  eva_c--;                                                   \
                }                                                            \
              if (eva_a < eva_c)                                             \
                {                                                            \
                  eva_tmp_swap = *eva_a;                                     \
                  *eva_a = *eva_c;                                           \
                  *eva_c = eva_tmp_swap;                                     \
                  if (eva_a == eva_b)                                        \
                    eva_b = eva_c;                                           \
                  else if (eva_b == eva_c)                                   \
                    eva_b = eva_a;                                           \
                  eva_a++;                                                   \
                  eva_c--;                                                   \
                }                                                            \
              else if (eva_a == eva_c)                                       \
                {                                                            \
                  eva_a++;                                                   \
                  eva_c--;                                                   \
                  break;                                                     \
                }                                                            \
            }                                                                \
          while (eva_a <= eva_c);                                            \
                                                                             \
          /* the two partitions are [lo,c] and [a,hi], */                    \
          /* which are disjoint since (a > b) by the above loop guard */     \
                                                                             \
          /*{type*eva_tmp2; for (eva_tmp2=eva_lo;eva_tmp2<=eva_c;eva_tmp2++){ compare(*eva_tmp2,*eva_b,eva_rv); g_assert(eva_rv<=0); }}*/ \
          /*{type*eva_tmp2; for (eva_tmp2=eva_a;eva_tmp2<=eva_hi;eva_tmp2++){ compare(*eva_tmp2,*eva_b,eva_rv); g_assert(eva_rv>=0); }}*/ \
          /*{type*eva_tmp2; for (eva_tmp2=eva_c+1;eva_tmp2<eva_a;eva_tmp2++){ compare(*eva_tmp2,*eva_b,eva_rv); g_assert(eva_rv==0); }}*/ \
                                                                             \
          /* push parts onto stack:  the bigger half must be pushed    */    \
          /* on first to guarantee that the max stack depth is O(log N) */   \
          eva_stack_nodes[0].start = eva_node.start;                         \
          eva_stack_nodes[0].len = eva_c - eva_lo + 1;                       \
          eva_stack_nodes[1].start = eva_a - (array);                        \
          eva_stack_nodes[1].len = eva_hi - eva_a + 1;                       \
          if (eva_stack_nodes[0].len < eva_stack_nodes[1].len)               \
            {                                                                \
              GskQsortStackNode eva_stack_node_tmp = eva_stack_nodes[0];     \
              eva_stack_nodes[0] = eva_stack_nodes[1];                       \
              eva_stack_nodes[1] = eva_stack_node_tmp;                       \
            }                                                                \
          if (eva_stack_nodes[0].len > isort_threshold)                      \
            {                                                                \
              if (eva_stack_nodes[1].len > isort_threshold)                  \
                {                                                            \
                  eva_stack[eva_stack_size++] = eva_stack_nodes[0];          \
                  eva_node = eva_stack_nodes[1];                             \
                }                                                            \
              else                                                           \
                {                                                            \
                  EVA_INSERTION_SORT ((array) + eva_stack_nodes[1].start,    \
                                      type, eva_stack_nodes[1].len, compare);\
                  eva_node = eva_stack_nodes[0];                             \
                }                                                            \
            }                                                                \
          else                                                               \
            {                                                                \
              EVA_INSERTION_SORT ((array) + eva_stack_nodes[0].start,        \
                                  type, eva_stack_nodes[0].len, compare);    \
              EVA_INSERTION_SORT ((array) + eva_stack_nodes[1].start,        \
                                  type, eva_stack_nodes[1].len, compare);    \
              if (eva_stack_size == 0)                                       \
                break;                                                       \
              eva_node = eva_stack[--eva_stack_size];                        \
            }                                                                \
          ss_assertion;                                                      \
        }                                                                    \
  }G_STMT_END

/* TODO: do we want EVA_INSERTION_SELECT for use here internally? */
#define EVA_QSELECT_FULL(array, type, n_elements, n_select, compare, isort_threshold, stack_size, ss_assertion)    \
  G_STMT_START{                                                              \
    gint eva_rv;                                                             \
    guint eva_stack_size;                                                    \
    GskQsortStackNode eva_stack[stack_size];                                 \
    type eva_tmp_swap;                                                       \
    GskQsortStackNode eva_node;                                              \
    eva_node.start = 0;                                                      \
    eva_node.len = (n_elements);                                             \
    eva_stack_size = 0;                                                      \
    if (n_elements <= isort_threshold)                                       \
      EVA_INSERTION_SORT(array,type,n_elements,compare);                     \
    else                                                                     \
      for(;;)                                                                \
        {                                                                    \
          GskQsortStackNode eva_stack_nodes[2];                              \
          /* implement median-of-three; sort so that  */                     \
          /* *eva_a <= *eva_b <= *eva_c               */                     \
          type *eva_lo = array + eva_node.start;                             \
          type *eva_hi = eva_lo + eva_node.len - 1;                          \
          type *eva_a = eva_lo;                                              \
          type *eva_b = array + eva_node.start + eva_node.len / 2;           \
          type *eva_c = eva_hi;                                              \
          compare((*eva_a), (*eva_b), eva_rv);                               \
          if (eva_rv < 0)                                                    \
            {                                                                \
              compare((*eva_b), (*eva_c), eva_rv);                           \
              if (eva_rv <= 0)                                               \
                {                                                            \
                  /* a <= b <= c: already sorted */                          \
                }                                                            \
              else                                                           \
                {                                                            \
                  compare((*eva_a), (*eva_c), eva_rv);                       \
                  if (eva_rv <= 0)                                           \
                    {                                                        \
                      /* a <= c <= b */                                      \
                      eva_tmp_swap = *eva_b;                                 \
                      *eva_b = *eva_c;                                       \
                      *eva_c = eva_tmp_swap;                                 \
                    }                                                        \
                  else                                                       \
                    {                                                        \
                      /* c <= a <= b */                                      \
                      eva_tmp_swap = *eva_a;                                 \
                      *eva_a = *eva_c;                                       \
                      *eva_c = *eva_b;                                       \
                      *eva_b = eva_tmp_swap;                                 \
                    }                                                        \
                }                                                            \
            }                                                                \
          else                                                               \
            {                                                                \
              /* *b < *a */                                                  \
              compare((*eva_b), (*eva_c), eva_rv);                           \
              if (eva_rv >= 0)                                               \
                {                                                            \
                  /* *c <= *b < *a */                                        \
                  eva_tmp_swap = *eva_c;                                     \
                  *eva_c = *eva_a;                                           \
                  *eva_a = eva_tmp_swap;                                     \
                }                                                            \
              else                                                           \
                {                                                            \
                  /* b<a, b<c */                                             \
                  compare((*eva_a), (*eva_c), eva_rv);                       \
                  if (eva_rv >= 0)                                           \
                    {                                                        \
                      /* b < c <= a */                                       \
                      eva_tmp_swap = *eva_a;                                 \
                      *eva_a = *eva_b;                                       \
                      *eva_b = *eva_c;                                       \
                      *eva_c = eva_tmp_swap;                                 \
                    }                                                        \
                  else                                                       \
                    {                                                        \
                      /* b < a < c */                                        \
                      eva_tmp_swap = *eva_a;                                 \
                      *eva_a = *eva_b;                                       \
                      *eva_b = eva_tmp_swap;                                 \
                    }                                                        \
                }                                                            \
            }                                                                \
                                                                             \
          /* ok, phew, now *eva_a <= *eva_b <= *eva_c */                     \
                                                                             \
          /* partition this range of the array */                            \
          eva_a++;                                                           \
          eva_c--;                                                           \
          do                                                                 \
            {                                                                \
              /* advance eva_a to a element that violates */                 \
              /* partitioning (or it hits eva_b) */                          \
              for (;;)                                                       \
                {                                                            \
                  compare((*eva_a), (*eva_b), eva_rv);                       \
                  if (eva_rv >= 0)                                           \
                    break;                                                   \
                  eva_a++;                                                   \
                }                                                            \
              /* advance eva_c to a element that violates */                 \
              /* partitioning (or it hits eva_b) */                          \
              for (;;)                                                       \
                {                                                            \
                  compare((*eva_b), (*eva_c), eva_rv);                       \
                  if (eva_rv >= 0)                                           \
                    break;                                                   \
                  eva_c--;                                                   \
                }                                                            \
              if (eva_a < eva_c)                                             \
                {                                                            \
                  eva_tmp_swap = *eva_a;                                     \
                  *eva_a = *eva_c;                                           \
                  *eva_c = eva_tmp_swap;                                     \
                  if (eva_a == eva_b)                                        \
                    eva_b = eva_c;                                           \
                  else if (eva_b == eva_c)                                   \
                    eva_b = eva_a;                                           \
                  eva_a++;                                                   \
                  eva_c--;                                                   \
                }                                                            \
              else if (eva_a == eva_c)                                       \
                {                                                            \
                  eva_a++;                                                   \
                  eva_c--;                                                   \
                  break;                                                     \
                }                                                            \
            }                                                                \
          while (eva_a <= eva_c);                                            \
                                                                             \
          /* the two partitions are [lo,c] and [a,hi], */                    \
          /* which are disjoint since (a > b) by the above loop guard */     \
                                                                             \
          /* push parts onto stack:  the bigger half must be pushed    */    \
          /* on first to guarantee that the max stack depth is O(log N) */   \
          eva_stack_nodes[0].start = eva_node.start;                         \
          eva_stack_nodes[0].len = eva_c - eva_lo + 1;                       \
          eva_stack_nodes[1].start = eva_a - (array);                        \
          eva_stack_nodes[1].len = eva_hi - eva_a + 1;                       \
          if (eva_stack_nodes[1].start >= n_select)                          \
            {                                                                \
              if (eva_stack_nodes[0].len > isort_threshold)                  \
                {                                                            \
                  eva_node = eva_stack_nodes[0];                             \
                }                                                            \
              else                                                           \
                {                                                            \
                  EVA_INSERTION_SORT ((array) + eva_stack_nodes[0].start,    \
                                      type, eva_stack_nodes[0].len, compare);\
                  if (eva_stack_size == 0)                                   \
                    break;                                                   \
                  eva_node = eva_stack[--eva_stack_size];                    \
                }                                                            \
            }                                                                \
          else                                                               \
            {                                                                \
              if (eva_stack_nodes[0].len < eva_stack_nodes[1].len)           \
                {                                                            \
                  GskQsortStackNode eva_stack_node_tmp = eva_stack_nodes[0]; \
                  eva_stack_nodes[0] = eva_stack_nodes[1];                   \
                  eva_stack_nodes[1] = eva_stack_node_tmp;                   \
                }                                                            \
              if (eva_stack_nodes[0].len > isort_threshold)                  \
                {                                                            \
                  if (eva_stack_nodes[1].len > isort_threshold)              \
                    {                                                        \
                      eva_stack[eva_stack_size++] = eva_stack_nodes[0];      \
                      eva_node = eva_stack_nodes[1];                         \
                      ss_assertion;                                          \
                    }                                                        \
                  else                                                       \
                    {                                                        \
                      EVA_INSERTION_SORT ((array) + eva_stack_nodes[1].start,\
                                          type, eva_stack_nodes[1].len, compare);\
                      eva_node = eva_stack_nodes[0];                         \
                    }                                                        \
                }                                                            \
              else                                                           \
                {                                                            \
                  EVA_INSERTION_SORT ((array) + eva_stack_nodes[0].start,    \
                                      type, eva_stack_nodes[0].len, compare);\
                  EVA_INSERTION_SORT ((array) + eva_stack_nodes[1].start,    \
                                  type, eva_stack_nodes[1].len, compare);    \
                  if (eva_stack_size == 0)                                   \
                    break;                                                   \
                  eva_node = eva_stack[--eva_stack_size];                    \
                }                                                            \
            }                                                                \
        }                                                                    \
  }G_STMT_END


/* note: do not allow equality, since that would make the next push a
   stack overflow, and we might not detect it correctly to stack corruption. */
#define EVA_QSORT_ASSERT_STACK_SIZE(stack_alloced)                          \
  g_assert(eva_stack_size < stack_alloced)

#define EVA_INSERTION_SORT(array, type, length, compare)                     \
  G_STMT_START{                                                              \
    guint eva_ins_i, eva_ins_j;                                              \
    type eva_ins_tmp;                                                        \
    for (eva_ins_i = 1; eva_ins_i < length; eva_ins_i++)                     \
      {                                                                      \
        /* move (eva_ins_i-1) into place */                                  \
        guint eva_ins_min = eva_ins_i - 1;                                   \
        gint eva_ins_compare_rv;                                             \
        for (eva_ins_j = eva_ins_i; eva_ins_j < length; eva_ins_j++)         \
          {                                                                  \
            compare(((array)[eva_ins_min]), ((array)[eva_ins_j]),            \
                    eva_ins_compare_rv);                                     \
            if (eva_ins_compare_rv > 0)                                      \
              eva_ins_min = eva_ins_j;                                       \
          }                                                                  \
        /* swap eva_ins_min and (eva_ins_i-1) */                             \
        eva_ins_tmp = (array)[eva_ins_min];                                  \
        (array)[eva_ins_min] = (array)[eva_ins_i - 1];                       \
        (array)[eva_ins_i - 1] = eva_ins_tmp;                                \
      }                                                                      \
  }G_STMT_END

#define EVA_QSORT_SIMPLE_COMPARATOR(a,b,compare_rv)                          \
  G_STMT_START{                                                              \
    if ((a) < (b))                                                           \
      compare_rv = -1;                                                       \
    else if ((a) > (b))                                                      \
      compare_rv = 1;                                                        \
    else                                                                     \
      compare_rv = 0;                                                        \
  }G_STMT_END
