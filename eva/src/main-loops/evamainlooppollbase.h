/*
    EVA - a library to write servers
    Copyright (C) 1999-2000 Dave Benson

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA

    Contact:
        daveb@ffem.org <Dave Benson>

*/

/* EvaMainLoopPollBase: A main loop based around a system call like poll(2)
 *    or select(2) that calls a `poll' function with a collection of file
 *    descriptors.  This class implements signal handling and process-end
 *    notification using the normal unix EINTR mechanism.
 */

/* NOTE: everything here is private, except EvaMainLoopPollBaseClass,
 *       which is protected. (this exposition is to permit derivation)
 */
 

#ifndef __EVA_MAIN_LOOP_POLL_BASE_H_
#define __EVA_MAIN_LOOP_POLL_BASE_H_


/* This is a derived class of EvaMainLoop that uses
 * poll(2) or select(2) or similar internally.
 */

/* Note: when deriving from this class,
 *       note that its finalize method will call `config_fd(0)'
 *       perhaps, so should chain the finalize method *first*
 */

#include "../evamainloop.h"
#include "../evabuffer.h"

G_BEGIN_DECLS

/* --- type macros --- */
GType eva_main_loop_poll_base_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_MAIN_LOOP_POLL_BASE			(eva_main_loop_poll_base_get_type ())
#define EVA_MAIN_LOOP_POLL_BASE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_MAIN_LOOP_POLL_BASE, EvaMainLoopPollBase))
#define EVA_MAIN_LOOP_POLL_BASE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_MAIN_LOOP_POLL_BASE, EvaMainLoopPollBaseClass))
#define EVA_MAIN_LOOP_POLL_BASE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_MAIN_LOOP_POLL_BASE, EvaMainLoopPollBaseClass))
#define EVA_IS_MAIN_LOOP_POLL_BASE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_MAIN_LOOP_POLL_BASE))
#define EVA_IS_MAIN_LOOP_POLL_BASE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_MAIN_LOOP_POLL_BASE))

/* --- Typedefs & structures --- */
typedef struct _EvaMainLoopPollBase EvaMainLoopPollBase;
typedef struct _EvaMainLoopPollBaseClass EvaMainLoopPollBaseClass;


/* --- EvaMainLoopPollBase structures --- */
struct _EvaMainLoopPollBaseClass 
{
  EvaMainLoopClass    main_loop_class;

  void              (*config_fd)       (EvaMainLoopPollBase   *main_loop,
                                        int                    fd,
					GIOCondition           old_io_conditions,
				        GIOCondition           io_conditions);

  /* returns FALSE if the poll function has an error.
   */
  gboolean          (*do_polling)      (EvaMainLoopPollBase   *main_loop,
				        int                    max_timeout,
				        guint                  max_events,
				        guint                 *num_events_out,
                                        EvaMainLoopEvent      *events);
};

struct _EvaMainLoopPollBase
{
  EvaMainLoop  	   main_loop;

  /*< private >*/

  /* signals that have been raised, as int's */
  EvaBuffer        signal_ids;

  /* process-termination notifications in the queue */
  EvaBuffer        process_term_notifications;

  /* a pipe which can be written to wake up the main-loop synchronously */
  EvaSource       *wakeup_read_pipe;
  gint             wakeup_read_fd;
  gint             wakeup_write_fd;
  
  /* whether we need to manually try waitpid() calls
     at the start of the next iteration. */
  guint try_waitpid : 1;
};

/* this function is multi-thread and signal safe! */
void eva_main_loop_poll_base_wakeup (EvaMainLoopPollBase *poll_base);

G_END_DECLS

#endif
