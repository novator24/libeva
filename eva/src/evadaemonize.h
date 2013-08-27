#ifndef __EVA_DAEMONIZE_H_
#define __EVA_DAEMONIZE_H_

#include <glib.h>

G_BEGIN_DECLS

typedef enum
{
  EVA_DAEMONIZE_FORK	= (1<<0),

  /* trap SIGILL, SIGABRT, SIGSEGV, SIGIOT, SIGBUG, SIGFPE (where available) */
  EVA_DAEMONIZE_RESTART_ERROR_SIGNALS = (1<<1),
  EVA_DAEMONIZE_SUPPORT_RESTART_EXIT_CODE = (1<<2)
} EvaDaemonizeFlags;

#define EVA_DAEMONIZE_DEFAULT_RESTART_EXIT_CODE         100

void eva_daemonize_set_defaults  (EvaDaemonizeFlags flags,
                                  guint             restart_exit_code);
void eva_daemonize_set_pid_filename (const char *filename);
void eva_daemonize_parse_options (int              *argc_inout,
                                  char           ***argv_inout);

/* this should be called after set_defaults,
   so that it can affect the printout. */
void eva_daemonize_print_options (void);
void eva_maybe_daemonize         (void);

G_END_DECLS

#endif
