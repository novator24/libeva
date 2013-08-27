#define USE_G_MAIN_LOOP	0

#include <glib.h>
#include "evamain.h"

/**
 * eva_main_run:
 * 
 * Run the main loop until it terminates, returning the value which 
 * should be returned by main().
 *
 * returns: the exit code for this process.
 */
#if USE_G_MAIN_LOOP
int eva_main_run ()
{
  GMainLoop *loop = g_main_loop_new (g_main_context_default (), FALSE);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);
}

#else	/* !USE_G_MAIN_LOOP */

#include "evamainloop.h"

int eva_main_run ()
{
  GskMainLoop *loop = eva_main_loop_default ();
  while (eva_main_loop_should_continue (loop))
    eva_main_loop_run (loop, -1, NULL);
  return loop->exit_status;
}

/**
 * eva_main_quit:
 * 
 * Quit the program by stopping eva_main_run().
 */
void
eva_main_quit ()
{
  GskMainLoop *loop = eva_main_loop_default ();
  eva_main_loop_quit (loop);
}

/**
 * eva_main_exit:
 * @exit_status: desired exit-status code for this process.
 * 
 * Exit the program by stopping eva_main_run().
 */
void
eva_main_exit (int exit_status)
{
  GskMainLoop *loop = eva_main_loop_default ();
  loop->exit_status = exit_status;
  eva_main_loop_quit (loop);
}

#endif	/* !USE_G_MAIN_LOOP */
