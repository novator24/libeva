#include "evafork.h"
#include "evaerrno.h"
#include "evaerror.h"
#include "evamacros.h"
#include "evamainloop.h"
#include <unistd.h>
#include <errno.h>


static GHashTable *fd_table = NULL;
G_LOCK_DEFINE_STATIC (fd_table);

/**
 * eva_fork:
 * @action: function to call from the background process.
 * @data: user-data to pass to @action.
 * @error: optional place to errors to be put.
 *
 * Fork a new thread, cleaning up loose file-descriptors,
 * then running the background function,
 * and exiting with the return value of the background function.
 *
 * returns: the process-id of the background thread,
 * or -1 on error.
 */
gint
eva_fork (GskForkFunc action,
	  gpointer    data,
	  GError    **error)
{
  int rv = fork ();
  if (rv < 0)
    {
      int e = errno;
      if (!eva_errno_is_ignorable (e))
	g_set_error (error, EVA_G_ERROR_DOMAIN,
		     eva_error_code_from_errno (e),
		     _("couldn't fork: %s"),
		     g_strerror (e));
    }
  else if (rv == 0)
    {
      eva_fork_child_cleanup ();
      _exit (action (data));
    }
  return rv;
}

/**
 * eva_fork_add_cleanup_fd:
 * @fd: a file descriptor that should be closed from a background thread.
 *
 * Add a file-descriptor to the list that should be closed
 * when fork() is run.
 *
 * A file-descriptor must only be added once.
 */
void
eva_fork_add_cleanup_fd    (int fd)
{
  G_LOCK (fd_table);
  if (fd_table == NULL)
    fd_table = g_hash_table_new (NULL, NULL);
#if ENABLE_CHECKS
  if (g_hash_table_lookup (fd_table, GUINT_TO_POINTER (fd)) != NULL)
    g_warning ("eva_fork_add_cleanup_fd: fd %d added twice", fd);
#endif
  g_hash_table_insert (fd_table, GUINT_TO_POINTER (fd), GUINT_TO_POINTER (1));
  G_UNLOCK (fd_table);
}

/**
 * eva_fork_remove_cleanup_fd:
 * @fd: remove a file descriptor that was previously registered
 * to be closed on fork with eva_fork_add_cleanup_fd().
 *
 * Remove a file-descriptor from the list that should be closed
 * when fork() is run.
 */
void
eva_fork_remove_cleanup_fd (int fd)
{
  G_LOCK (fd_table);
#if ENABLE_CHECKS
  if (fd_table == NULL
   || g_hash_table_lookup (fd_table, GUINT_TO_POINTER (fd)) == NULL)
    g_warning ("eva_fork_remove_cleanup_fd: fd %d removed spuriously", fd);
#endif
  g_hash_table_remove (fd_table, GUINT_TO_POINTER (fd));
  G_UNLOCK (fd_table);
}

static void close_fd_as_ptr (gpointer fd_ptr)
{
  int fd = GPOINTER_TO_UINT (fd_ptr);
  close (fd);
}

/**
 * eva_fork_child_cleanup:
 *
 * Do all cleanup that should follow a fork(2) system call
 * from the child process.
 */
void
eva_fork_child_cleanup(void)
{
  if (fd_table != NULL)
    {
      g_hash_table_foreach (fd_table, (GHFunc) close_fd_as_ptr, NULL);
      g_hash_table_destroy (fd_table);
      fd_table = NULL;
    }
  _eva_main_loop_fork_notify ();
}
