#ifndef __EVA_TABLE_H_
#define __EVA_TABLE_H_

/* EvaTable.
 *
 * A efficient on-disk table, meaning a mapping from an
 * uninterpreted piece of binary-data to a value.
 * When multiple values are added with the same
 * key, they are merged, using user-definable semantics.
 *
 * application notes:
 * - you may store data in the directory, but it the filename
 *   must begin with a capital letter.
 */
typedef struct _EvaTableReader EvaTableReader;

#include "evamempool.h"

typedef struct
{
  guint len;
  guint8 *data;
  guint alloced;
} EvaTableBuffer;

/* --- public table buffer api -- needed for defining merge functions --- */
/* calls realloc so beginning of buffer is preserved */
G_INLINE_FUNC guint8 *eva_table_buffer_set_len (EvaTableBuffer *buffer,
                                                guint           len);

/* returns pointer to new area in buffer */
G_INLINE_FUNC guint8 *eva_table_buffer_append  (EvaTableBuffer *buffer,
                                                guint           len);


typedef  int (*EvaTableCompareFunc)      (guint         a_key_len,
                                          const guint8 *a_key_data,
                                          guint         b_key_len,
                                          const guint8 *b_key_data,
                                          gpointer      user_data);
typedef  int (*EvaTableCompareFuncNoLen) (const guint8 *a_key_data,
                                          const guint8 *b_key_data,
                                          gpointer      user_data);
typedef enum
{
  EVA_TABLE_MERGE_RETURN_A,
  EVA_TABLE_MERGE_RETURN_B,
  EVA_TABLE_MERGE_SUCCESS,
  EVA_TABLE_MERGE_DROP,
} EvaTableMergeResult;
typedef EvaTableMergeResult (*EvaTableMergeFunc) (guint         key_len,
                                                  const guint8 *key_data,
                                                  guint         a_len,
                                                  const guint8 *a_data,
                                                  guint         b_len,
                                                  const guint8 *b_data,
                                                  EvaTableBuffer *output,
                                                  gpointer      user_data);
typedef EvaTableMergeResult (*EvaTableMergeFuncNoLen) (const guint8 *key_data,
                                                       const guint8 *a_data,
                                                       const guint8 *b_data,
                                                       EvaTableBuffer *output,
                                                       gpointer      user_data);

/* used for merges that go back to the beginning of the indexer */
typedef enum
{
  EVA_TABLE_SIMPLIFY_IDENTITY,
  EVA_TABLE_SIMPLIFY_SUCCESS,
  EVA_TABLE_SIMPLIFY_DELETE
} EvaTableSimplifyResult;
typedef EvaTableSimplifyResult (*EvaTableSimplifyFunc)(guint         key_len,
                                                       const guint8 *key_data,
                                                       guint         value_len,
                                                       const guint8 *value_data,
                                                       EvaTableBuffer*val_out,
                                                       gpointer      user_data);
typedef EvaTableSimplifyResult (*EvaTableSimplifyFuncNoLen)
                                                      (const guint8 *key_data,
                                                       const guint8 *value_data,
                                                       EvaTableBuffer*val_out,
                                                       gpointer      user_data);

/* only used for querying */
typedef gboolean          (*EvaTableValueIsStableFunc)(guint         key_len,
                                                       const guint8 *key_data,
                                                       guint         value_len,
                                                       const guint8 *value_data,
                                                       gpointer      user_data);

typedef enum
{
  EVA_TABLE_JOURNAL_NONE,
  EVA_TABLE_JOURNAL_OCCASIONALLY,
  EVA_TABLE_JOURNAL_DEFAULT
} EvaTableJournalMode;

typedef struct _EvaTableOptions EvaTableOptions;
struct _EvaTableOptions
{
  /* compare configuration */
  EvaTableCompareFunc compare;
  EvaTableCompareFuncNoLen compare_no_len;

  /* merging configuration */
  EvaTableMergeFunc merge;
  EvaTableMergeFuncNoLen merge_no_len;

  /* final merging */
  EvaTableSimplifyFunc simplify;
  EvaTableSimplifyFuncNoLen simplify_no_len;

  /* query stability */
  EvaTableValueIsStableFunc is_stable;

  /* user data */
  gpointer user_data;
  GDestroyNotify destroy_user_data;

  /* journalling mode */
  EvaTableJournalMode journal_mode;

  /* tunables */
  gsize max_in_memory_entries;
  gsize max_in_memory_bytes;
};

EvaTableOptions     *eva_table_options_new    (void);
void                 eva_table_options_destroy(EvaTableOptions *options);

void eva_table_options_set_replacement_semantics (EvaTableOptions *options);

typedef enum
{
  EVA_TABLE_MAY_EXIST = (1<<0),
  EVA_TABLE_MAY_CREATE = (1<<1)
} EvaTableNewFlags;

typedef struct _EvaTable EvaTable;

/* NOTE: hinting options will be ignored if the table already exists. */
EvaTable *  eva_table_new         (const char            *dir,
                                   const EvaTableOptions *options,
                                   EvaTableNewFlags       flags,
	          	           GError               **error);
gboolean    eva_table_add         (EvaTable              *table,
                                   guint                  key_len,
	          	           const guint8          *key_data,
                                   guint                  value_len,
	          	           const guint8          *value_data,
                                   GError               **error);
gboolean    eva_table_query       (EvaTable              *table,
                                   guint                  key_len,
			           const guint8          *key_data,
                                   gboolean              *found_value_out,
			           guint                 *value_len_out,
			           guint8               **value_data_out,
                                   GError               **error);
const char *eva_table_peek_dir    (EvaTable              *table);
void        eva_table_destroy     (EvaTable              *table);


struct _EvaTableReader
{
  gboolean eof;
  GError *error;
  guint key_len;
  const guint8 *key_data;
  guint value_len;
  const guint8 *value_data;

  void     (*advance) (EvaTableReader *reader);
  void     (*destroy) (EvaTableReader *reader);
};
typedef enum
{
  EVA_TABLE_BOUND_NONE,
  EVA_TABLE_BOUND_STRICT,
  EVA_TABLE_BOUND_INCLUSIVE
} EvaTableBoundType;
EvaTableReader *eva_table_make_reader_with_bounds (EvaTable *table,
                                       EvaTableBoundType start_bound_type,
                                       guint start_bound_len,
                                       const guint8 *start_bound_data,
                                       EvaTableBoundType end_bound_type,
                                       guint end_bound_len,
                                       const guint8 *end_bound_data,
                                       GError  **error);
#define eva_table_make_reader(table, error) \
  eva_table_make_reader_with_bounds (table, \
                                     EVA_TABLE_BOUND_NONE, 0, NULL, \
                                     EVA_TABLE_BOUND_NONE, 0, NULL, \
                                     error)

G_INLINE_FUNC void     eva_table_reader_advance (EvaTableReader *reader);
G_INLINE_FUNC void     eva_table_reader_destroy (EvaTableReader *reader);





/* --- protected parts of the table-buffer api --- */
/* this stuff is used throughout the implementation:
   we also cavalierly access 'len', 'data' and 'alloced'
   and assume that they have the expected properties.
   we do not modify 'data' or 'alloced' and we always
   ensure that len < alloced. */

#define EVA_TABLE_BUFFER_INIT   { 0, NULL, 0 }
G_INLINE_FUNC void    eva_table_buffer_init         (EvaTableBuffer *buffer);

G_INLINE_FUNC void    eva_table_buffer_clear        (EvaTableBuffer *buffer);

G_INLINE_FUNC void    eva_table_buffer_ensure_size  (EvaTableBuffer *buffer,
                                                     guint           min_size);
G_INLINE_FUNC void    eva_table_buffer_ensure_extra (EvaTableBuffer *buffer,
                                                     guint           addl_size);


#if defined (G_CAN_INLINE) || defined (__EVA_DEFINE_INLINES__)
G_INLINE_FUNC void    eva_table_buffer_init    (EvaTableBuffer *buffer)
{
  buffer->len = 0;
  buffer->data = NULL;
  buffer->alloced = 0;
}

/* calls realloc so beginning of buffer is preserved */
G_INLINE_FUNC void
eva_table_buffer_ensure_size (EvaTableBuffer *buffer,
                              guint           min_size)
{
  if (G_UNLIKELY (buffer->alloced < min_size))
    {
      guint new_size = buffer->alloced ? buffer->alloced * 2 : 32;
      while (new_size < min_size)
        new_size += new_size;
      buffer->data = g_realloc (buffer->data, new_size);
      buffer->alloced = new_size;
    }
}

G_INLINE_FUNC guint8 *
eva_table_buffer_set_len (EvaTableBuffer *buffer,
                          guint           len)
{
  eva_table_buffer_ensure_size (buffer, len);
  buffer->len = len;
  return buffer->data;
}

/* returns pointer to new area in buffer */
G_INLINE_FUNC guint8 *
eva_table_buffer_append  (EvaTableBuffer *buffer,
                          guint           len)
{
  guint old_len = buffer->len;
  return eva_table_buffer_set_len (buffer, old_len + len) + old_len;
}

G_INLINE_FUNC void
eva_table_buffer_clear   (EvaTableBuffer *buffer)
{
  g_free (buffer->data);
}

G_INLINE_FUNC void
eva_table_buffer_ensure_extra (EvaTableBuffer *buffer,
                               guint           extra_bytes)
{
  eva_table_buffer_ensure_size (buffer, buffer->len + extra_bytes);
}
G_INLINE_FUNC void
eva_table_reader_advance (EvaTableReader *reader)
{
  reader->advance (reader);
}
G_INLINE_FUNC void
eva_table_reader_destroy (EvaTableReader *reader)
{
  reader->destroy (reader);
}
#endif

#endif
