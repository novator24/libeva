#ifndef __EVA_G_HELPER_H_
#define __EVA_G_HELPER_H_

#include <glib.h>

G_BEGIN_DECLS

void eva_g_ptr_array_foreach (GPtrArray *array,
			      GFunc      func,
			      gpointer   data);

void eva_g_error_add_prefix  (GError   **error,
                              const char *format,
                              ...) G_GNUC_PRINTF(2,3);


gpointer eva_g_tree_min (GTree *tree);
gpointer eva_g_tree_max (GTree *tree);

GSList *eva_g_tree_key_slist (GTree *tree);
GSList *eva_g_tree_value_slist (GTree *tree);
GSList *eva_g_hash_table_key_slist (GHashTable *table);
GSList *eva_g_hash_table_value_slist (GHashTable *table);

/* MOVE TO `evastr.h' ??? */
/* semi-portable 64-bit int parsing functions */
gint64  eva_strtoll  (const char *str,
		      char      **endp,
		      int         base);
guint64  eva_strtoull(const char *str,
		      char      **endp,
		      int         base);

guint eva_strnlen (const char *ptr, guint max_len);

/* MOVE ELSEWHERE ??? */
gboolean eva_fd_set_nonblocking (int fd);
gboolean eva_fd_clear_nonblocking (int fd);
gboolean eva_fd_is_nonblocking (int fd);
/* NOTE: eva_fd_finish_connecting() is eva_socket_address_finish_fd(). */

#ifndef EVA_DISABLE_DEPRECATED
/* eva_g_debug: write debug output */
#define eva_g_debug g_debug
#endif

G_END_DECLS

#endif
