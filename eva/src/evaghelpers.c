#include "evaghelpers.h"
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "config.h"

/* --- hashtable => lists --- */

/**
 * eva_g_ptr_array_foreach:
 * @array: array to iterate over.
 * @func: function to call on each element of @array.
 * @data: second parameter to @func.
 *
 * Like g_slist_foreach(), except it iterates over a GPtrArray instead.
 */
void eva_g_ptr_array_foreach (GPtrArray *array,
			      GFunc      func,
			      gpointer   data)
{
  guint i;
  for (i = 0; i < array->len; i++)
    (*func) (array->pdata[i], data);
}

static gint
min_search_func (gconstpointer       key,
		 gconstpointer       data)
{
  * (gpointer *) data = (gpointer) key;
  return -1;
}

/**
 * eva_g_tree_min:
 * @tree: tree to examine.
 *
 * Find the minimum key in the tree.
 *
 * returns: smallest key in the tree.
 */
gpointer eva_g_tree_min (GTree *tree)
{
  gpointer rv = NULL;
  g_tree_search (tree, min_search_func, &rv);
  return rv;
}

static gint
max_search_func (gconstpointer       key,
		 gconstpointer       data)
{
  * (gpointer *) data = (gpointer) key;
  return +1;
}

/**
 * eva_g_tree_max:
 * @tree: tree to examine.
 *
 * Find the maximum key in the tree.
 *
 * returns: largest key in the tree.
 */
gpointer eva_g_tree_max (GTree *tree)
{
  gpointer rv = NULL;
  g_tree_search (tree, max_search_func, &rv);
  return rv;
}

static gint
prepend_key_pointer (gpointer         key,
		     gpointer         value,
		     gpointer         data)
{
  GSList **plist = data;
  *plist = g_slist_prepend (*plist, key);
  return 1;
}

static gint
prepend_value_pointer (gpointer         key,
		       gpointer         value,
		       gpointer         data)
{
  GSList **plist = data;
  *plist = g_slist_prepend (*plist, value);
  return 1;
}

/**
 * eva_g_tree_key_slist:
 * @tree: the tree to scan.
 *
 * Get a list of all the keys in the trees, sorted.
 *
 * returns: the newly allocated list of pointers to the keys.
 */
GSList *eva_g_tree_key_slist (GTree *tree)
{
  GSList *rv = NULL;
  g_tree_traverse (tree, prepend_key_pointer, G_IN_ORDER, &rv);
  return g_slist_reverse (rv);
}

/**
 * eva_g_tree_value_slist:
 * @tree: the tree to scan.
 *
 * Get a list of all the values in the trees, sorted by key.
 *
 * returns: the newly allocated list of pointers to the keys.
 */
GSList *eva_g_tree_value_slist (GTree *tree)
{
  GSList *rv = NULL;
  g_tree_traverse (tree, prepend_value_pointer, G_IN_ORDER, &rv);
  return g_slist_reverse (rv);
}

/**
 * eva_g_hash_table_key_slist:
 * @table: the hash table to scan.
 *
 * Accumulate all the keys in the hash table into a big list.
 * You may not assume anything about the order of the list.
 *
 * returns: an allocated GSList of all the keys in the hash-table.
 */
GSList *eva_g_hash_table_key_slist (GHashTable *table)
{
  GSList *rv = NULL;
  g_hash_table_foreach (table, (GHFunc) prepend_key_pointer, &rv);
  return rv;
}

/**
 * eva_g_hash_table_value_slist:
 * @table: the hash table to scan.
 *
 * Accumulate all the values in the hash table into a big list.
 * You may not assume anything about the order of the list.
 *
 * returns: an allocated GSList of all the values in the hash-table.
 */
GSList *eva_g_hash_table_value_slist (GHashTable *table)
{
  GSList *rv = NULL;
  g_hash_table_foreach (table, (GHFunc) prepend_value_pointer, &rv);
  return rv;
}

/**
 * eva_g_error_add_prefix_valist:
 * @error: the pointer to error to change.
 * @format: printf-like format string.
 * @args: printf-like arguments.
 *
 * If @error is non-NULL, and points to a GError,
 * then modify it to be a GError that has the given
 * printf'd string as a prefix.
 */
void
eva_g_error_add_prefix_valist  (GError   **error,
                                const char *format,
                                va_list     args)
{
  if (error && *error)
    {
      char *s = g_strdup_vprintf (format, args);
      GError *e = g_error_new ((*error)->domain,(*error)->code,
                               "%s: %s", s, (*error)->message);
      g_free (s);
      g_error_free (*error);
      *error = e;
    }
}

/**
 * eva_g_error_add_prefix:
 * @error: the pointer to error to change.
 * @format: printf-like format string.
 * @...: printf-like arguments.
 *
 * If @error is non-NULL, and points to a GError,
 * then modify it to be a GError that has the given
 * printf'd string as a prefix.
 */
void
eva_g_error_add_prefix  (GError   **error,
                         const char *format,
                         ...)
{
  va_list args;
  va_start (args, format);
  eva_g_error_add_prefix_valist (error, format, args);
  va_end (args);
}

/**
 * eva_strtoll:
 * @str: the string to parse a longlong integer (gint64) from.
 * @endp: optional place to store the character right past
 * the number in @str.  If *endp == @str, then you may assume an error
 * occurred.
 * @base: the assumed base for the number to parse.  eg "2" to
 * parse a binary, "8" for octal, "10" for decimal, "16" for hexidecimal.
 * Also, "0" is autodetects the C-style base.
 *
 * Like strtol, but for 64-bit integers.
 *
 * returns: the parsed integer.
 */
gint64
eva_strtoll  (const char *str,
	      char      **endp,
	      int         base)
{
#if HAVE_STRTOQ
  return strtoq (str, endp, base);
#elif HAVE_STRTOLL
  return strtoll (str, endp, base);
#else
  return strtol (str, endp, base);
#endif
}

/**
 * eva_strtoull:
 * @str: the string to parse a longlong unsigned integer (guint64) from.
 * @endp: optional place to store the character right past
 * the number in @str.  If *endp == @str, then you may assume an error
 * occurred.
 * @base: the assumed base for the number to parse.  eg "2" to
 * parse a binary, "8" for octal, "10" for decimal, "16" for hexidecimal.
 * Also, "0" is autodetects the C-style base.
 *
 * Like strtol, but for 64-bit unsigned integers.
 *
 * returns: the parsed unsigned integer.
 */
guint64
eva_strtoull (const char *str,
	      char      **endp,
	      int         base)
{
#if HAVE_STRTOUQ
  return strtouq (str, endp, base);
#elif HAVE_STRTOULL
  return strtoull (str, endp, base);
#else
  return strtoul (str, endp, base);
#endif
}

/**
 * eva_strnlen:
 * @ptr: the string to find the length of.
 * @max_len: the maximum length the string could be.
 *
 * Find the length of a string, which is only allocated
 * @max_len bytes, and does not need to be NUL-terminated.
 *
 * returns: the length.
 */
guint
eva_strnlen (const char *ptr, guint max_len)
{
  guint rv = 0;
  while (max_len-- != 0 && *ptr++ != 0)
    rv++;
  return rv;
}

/**
 * eva_fd_set_nonblocking:
 * @fd: the file-descriptor to make non-blocking.
 *
 * Make a file-descriptor non-blocking.
 * When it is non-blocking, operations that would
 * cause it to block should instead return
 * EAGAIN.  (Although you should use eva_errno_is_ignorable()
 * to test, since windoze uses a different code, EWOULDBLOCK,
 * and you should always ignore EINTR.)
 *
 * returns: whether it was able to set the file descriptor non-blocking.
 */
gboolean
eva_fd_set_nonblocking(int fd)
{
  int flags = fcntl (fd, F_GETFL);
  if (flags < 0)
    return FALSE;
  fcntl (fd, F_SETFL, flags | O_NONBLOCK);
  return TRUE;
}

gboolean
eva_fd_clear_nonblocking(int fd)
{
  int flags = fcntl (fd, F_GETFL);
  if (flags < 0)
    return FALSE;
  fcntl (fd, F_SETFL, flags & (~O_NONBLOCK));
  return TRUE;
}

gboolean
eva_fd_is_nonblocking(int fd)
{
  int flags = fcntl (fd, F_GETFL);
  if (flags < 0)
    return FALSE;
  return (flags & O_NONBLOCK) == O_NONBLOCK;
}
