#ifndef __EVA_ERRNO_H_
#define __EVA_ERRNO_H_

#include <glib.h>

G_BEGIN_DECLS

gboolean eva_errno_is_ignorable (int errno_value);
int eva_errno_from_fd (int fd);


/* --- Handling Out-Of-File-Descriptor Issues --- */

/* this should be called called whenever 
 * a file-descriptor could not be created.
 * It does not affect 'errno',
 * but it uses it. */
void eva_errno_fd_creation_failed (void);

/* this can be called instead if you have 'errno' saved
   in a variable.  it may change 'errno'. */
void eva_errno_fd_creation_failed_errno (int e);

/* mechanism to get notification when the file-descriptors
   run out.  default behavior is to abort. */
typedef void (*EvaErrnoFdCreateFailedFunc) (gboolean system_wide);
void eva_errno_trap_fd_creation_failed (EvaErrnoFdCreateFailedFunc);



G_END_DECLS

#endif
