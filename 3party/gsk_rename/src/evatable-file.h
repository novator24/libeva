/* EvaTableFile:
 *  A mechanism for creating searchable files from a sorted stream of entrys.
 */

typedef struct _EvaTableFileFactory EvaTableFileFactory;
typedef struct _EvaTableFile EvaTableFile;
typedef struct _EvaTableFileHints EvaTableFileHints;


struct _EvaTableFileHints
{
  guint64 max_entries;
  guint64 max_key_bytes;
  guint64 max_value_bytes;
  guint min_key_size, max_key_size;
  guint min_value_size, max_value_size;
  gboolean allocate_disk_space_based_on_max_sizes;
};
#define EVA_TABLE_FILE_HINTS_DEFAULTS \
{ G_MAXUINT64, G_MAXUINT64, G_MAXUINT64, \
  0, G_MAXUINT, 0, G_MAXUINT,  \
  FALSE  /* allocate_disk_space_based_on_max_sizes */ \
}


struct _EvaTableFile
{
  EvaTableFileFactory *factory;
  guint64 id;

  /* NOTE: n_entries must be set by create_file() and open_building_file()
     implementations, but must be set by caller for open_file().
     (ugly, but it's internal api) */
  guint64 n_entries;
};

typedef struct _EvaTableFileQuery EvaTableFileQuery;
struct _EvaTableFileQuery
{
  /* returns the implicit key <=> test_key; the implicit key is setup by
     the caller somewhere in compare_data */
  gint (*compare) (guint         test_key_len,
                   const guint8 *test_key,
                   gpointer      compare_data);
  gpointer compare_data;

  gboolean found;
  EvaTableBuffer value;
};

#define EVA_TABLE_FILE_QUERY_INIT  { NULL, NULL, FALSE, EVA_TABLE_BUFFER_INIT }

G_INLINE_FUNC void    eva_table_file_query_clear (EvaTableFileQuery *query);

/* Copy "dir" as passed into create_file(), open_building_file(), etc.
   This isn't necessary b/c the "dir" is always a member of the
   EvaTable, and therefore will always be destroyed AFTER the files. */
#define EVA_TABLE_FILE_COPY_DIR            0

#if EVA_TABLE_FILE_COPY_DIR
# define EVA_TABLE_FILE_DIR_MAYBE_STRDUP(dir) g_strdup(dir)
# define EVA_TABLE_FILE_DIR_MAYBE_FREE(dir)   g_free((char*)(dir))
#else
# define EVA_TABLE_FILE_DIR_MAYBE_STRDUP(dir) dir
# define EVA_TABLE_FILE_DIR_MAYBE_FREE(dir)   do {} while(0)
#endif

typedef enum
{
  EVA_TABLE_FEED_ENTRY_WANT_MORE,
  EVA_TABLE_FEED_ENTRY_SUCCESS,
  EVA_TABLE_FEED_ENTRY_ERROR
} EvaTableFeedEntryResult;

struct _EvaTableFileFactory
{
  /* creating files (in initial, mid-build and queryable states) */
  EvaTableFile     *(*create_file)      (EvaTableFileFactory      *factory,
					 const char               *dir,
					 guint64                   id,
				         const EvaTableFileHints  *hints,
			                 GError                  **error);
  EvaTableFile     *(*open_building_file)(EvaTableFileFactory     *factory,
					 const char               *dir,
					 guint64                   id,
                                         guint                     state_len,
                                         const guint8             *state_data,
			                 GError                  **error);
  EvaTableFile     *(*open_file)        (EvaTableFileFactory      *factory,
					 const char               *dir,
					 guint64                   id,
			                 GError                  **error);

  /* methods for a file which is being built */
  EvaTableFeedEntryResult
                    (*feed_entry)      (EvaTableFile             *file,
                                         guint                     key_len,
                                         const guint8             *key_data,
                                         guint                     value_len,
                                         const guint8             *value_data,
					 GError                  **error);
  gboolean          (*done_feeding)     (EvaTableFile             *file,
                                         gboolean                 *ready_out,
					 GError                  **error);
  gboolean          (*get_build_state)  (EvaTableFile             *file,
                                         guint                    *state_len_out,
                                         guint8                  **state_data_out,
					 GError                  **error);
  gboolean          (*build_file)       (EvaTableFile             *file,
                                         gboolean                 *ready_out,
					 GError                  **error);
  void              (*release_build_data)(EvaTableFile            *file);

  /* methods for a file which has been constructed;
     some file types can be queried before they are constructed */
  gboolean          (*query_file)       (EvaTableFile             *file,
                                         EvaTableFileQuery        *query_inout,
					 GError                  **error);
  EvaTableReader   *(*create_reader)    (EvaTableFile             *file,
                                         const char               *dir,
                                         GError                  **error);
  /* you must always be able to get reader state */
  gboolean          (*get_reader_state) (EvaTableFile             *file,
                                         EvaTableReader           *reader,
                                         guint                    *state_len_out,
                                         guint8                  **state_data_out,
                                         GError                  **error);
  EvaTableReader   *(*recreate_reader)  (EvaTableFile             *file,
                                         const char               *dir,
                                         guint                     state_len,
                                         const guint8             *state_data,
                                         GError                  **error);

  /* destroying files and factories */
  gboolean          (*destroy_file)     (EvaTableFile             *file,
                                         const char               *dir,
                                         gboolean                  erase,
					 GError                  **error);
  void              (*destroy_factory)  (EvaTableFileFactory      *factory);
};

/* optimized for situations where writes predominate */
EvaTableFileFactory *eva_table_file_factory_new_flat (void);

/* optimized for situations where reads are common */
EvaTableFileFactory *eva_table_file_factory_new_btree (void);

#define eva_table_file_factory_create_file(factory, dir, id, hints, error) \
  ((factory)->create_file ((factory), (dir), (id), (hints), (error)))
#define eva_table_file_factory_open_file(factory, dir, id, error) \
  ((factory)->open_file ((factory), (dir), (id), (error)))
#define eva_table_file_factory_open_building_file(factory, dir, id, state_len, state_data, error) \
  ((factory)->open_building_file ((factory), (dir), (id), (state_len), (state_data), (error)))
#define eva_table_file_feed_entry(file, key_len, key_data, value_len, value_data, error) \
  ((file)->factory->feed_entry ((file), key_len, key_data, value_len, value_data, error))
#define eva_table_file_get_build_state(file, state_len_out, state_data_out, error) \
  ((file)->factory->get_build_state ((file), (state_len_out), (state_data_out), (error)))
#define eva_table_file_done_feeding(file, ready_out, error) \
  ((file)->factory->done_feeding ((file), (ready_out), (error)))
#define eva_table_file_build_file(file, ready_out, error) \
  ((file)->factory->build_file ((file), (ready_out), (error)))
#define eva_table_file_query(file, query_inout, error) \
  ((file)->factory->query_file ((file), (query_inout), (error)))
#define eva_table_file_create_reader(file, dir, error) \
  ((file)->factory->create_reader ((file), (dir), (error)))
#define eva_table_file_get_reader_state(file, reader, state_len_out, state_out, error) \
  ((file)->factory->get_reader_state ((file), (reader), (state_len_out), (state_out), (error)))
#define eva_table_file_recreate_reader(file, dir, state_len, state_data, error) \
  ((file)->factory->recreate_reader ((file), (dir), (state_len), (state_data), (error)))
#define eva_table_file_destroy(file, dir, erase, error) \
  ((file)->factory->destroy_file ((file), (dir), (erase), (error)))
#define eva_table_file_factory_destroy(factory) \
  (factory)->destroy_factory (factory)

#if defined (G_CAN_INLINE) || defined (__EVA_DEFINE_INLINES__)
G_INLINE_FUNC void    eva_table_file_query_clear (EvaTableFileQuery *query)
{
  eva_table_buffer_clear (&query->value);
}
#endif


