#ifndef __EVA_MAIN_H_
#define __EVA_MAIN_H_

#include <glib.h>

G_BEGIN_DECLS

int eva_main_run ();
void eva_main_quit ();
void eva_main_exit (int exit_status);

G_END_DECLS

#endif
