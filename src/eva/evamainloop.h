/*
    EVA - a library to write servers

    Copyright (C) 2001 Dave Benson

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

#ifndef __EVA_MAIN_LOOP_H_
#define __EVA_MAIN_LOOP_H_

#include <glib-object.h>

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _EvaMainLoopChange EvaMainLoopChange;
typedef struct _EvaMainLoopEvent EvaMainLoopEvent;
typedef struct _EvaMainLoopClass EvaMainLoopClass;
typedef struct _EvaMainLoop EvaMainLoop;
typedef struct _EvaMainLoopWaitInfo EvaMainLoopWaitInfo;
typedef struct _EvaSource EvaSource;
typedef struct _EvaMainLoopContextList EvaMainLoopContextList;

/* --- type macros --- */
GType eva_main_loop_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_MAIN_LOOP		(eva_main_loop_get_type ())
#define EVA_MAIN_LOOP(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_MAIN_LOOP, EvaMainLoop))
#define EVA_MAIN_LOOP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_MAIN_LOOP, EvaMainLoopClass))
#define EVA_MAIN_LOOP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_MAIN_LOOP, EvaMainLoopClass))
#define EVA_IS_MAIN_LOOP(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_MAIN_LOOP))
#define EVA_IS_MAIN_LOOP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_MAIN_LOOP))

GType eva_main_loop_wait_info_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_MAIN_LOOP_WAIT_INFO    (eva_main_loop_wait_info_get_type ())
struct _EvaMainLoopWaitInfo
{
  int               pid; 
  gboolean          exited;         /* exit(2) or killed by signal? */
  union {
    int             signal;         /* !exited */
    int             exit_status;    /*  exited */
  } d;           
  gboolean          dumped_core;
};
/* HINT: for diagnosing processes that die by unexpected signals,
   use g_strsignal() to convert signal numbers to strings */

typedef enum
{
  EVA_MAIN_LOOP_EVENT_IO,
  EVA_MAIN_LOOP_EVENT_SIGNAL,
  EVA_MAIN_LOOP_EVENT_PROCESS
} EvaMainLoopEventType;

struct _EvaMainLoopChange
{
  EvaMainLoopEventType type;
  union
  {
    struct {
      guint number;
      gboolean add;
    } signal;
    struct {
      guint fd;
      GIOCondition old_events;
      GIOCondition events;
    } io;
    struct {
      gint pid;
      gboolean add;
      gboolean did_exit;
    } process;
  } data;
};

struct _EvaMainLoopEvent
{
  EvaMainLoopEventType type;
  union
  {
    guint signal;
    struct {
      guint fd;
      GIOCondition events;
    } io;
    EvaMainLoopWaitInfo process_wait_info;
  } data;
};

/* --- structures --- */
struct _EvaMainLoopClass 
{
  GObjectClass object_class;
  gboolean (*setup)  (EvaMainLoop       *main_loop);
  void     (*change) (EvaMainLoop       *main_loop,
                      EvaMainLoopChange *change);
  guint    (*poll)   (EvaMainLoop       *main_loop,
                      guint              max_events_out,
                      EvaMainLoopEvent  *events,
                      gint               timeout);
};

struct _EvaMainLoop 
{
  GObject      object;

  /* idle functions */
  EvaSource     *first_idle;
  EvaSource     *last_idle;

  /* timers */
  EvaSource     *timers;

  /* i/o handlers by file-descriptor */
  GPtrArray     *read_sources;
  GPtrArray     *write_sources;

  /* lists of sources for each signal */
  GPtrArray     *signal_source_lists;

  /* process-termination handlers (int => (GSList<EvaSource>)) */
  GHashTable    *process_source_lists;
  GHashTable    *alive_pids;

  /* the source which is currently running */
  EvaSource     *running_source;

  GTimeVal       current_time;

  /* optional thread pool support */
  guint          max_workers;
  gpointer       thread_pool;

  guint          num_sources;
  guint          is_setup : 1;
  guint          is_running : 1;		/*< private >*/
  guint          quit : 1;			/*< public >*/

  gint		 exit_status;

  EvaMainLoopEvent *event_array_cache;
  unsigned       max_events;

  /* a list of GMainContext's */
  EvaMainLoopContextList *first_context;
  EvaMainLoopContextList *last_context;
};

/* --- Callback function typedefs. --- */

/* callback for child-process termination */
typedef void     (*EvaMainLoopWaitPidFunc)(EvaMainLoopWaitInfo  *info,
                                           gpointer              user_data);

/* callback for an "idle" function -- it runs after all events
 * have been processed.
 */
typedef gboolean (*EvaMainLoopIdleFunc)   (gpointer              user_data);

/* callback for receiving a signal */
typedef gboolean (*EvaMainLoopSignalFunc) (int                   sig_no,
                                           gpointer              user_data);

/* callback for a period */
typedef gboolean (*EvaMainLoopTimeoutFunc)(gpointer              user_data);

/* callback for input or output on a file descriptor */
typedef gboolean (*EvaMainLoopIOFunc)     (int                   fd,
                                           GIOCondition          condition,
                                           gpointer              user_data);


/* --- prototypes --- */
/* Create a main loop with selected options. */
typedef enum
{
  EVA_MAIN_LOOP_NEEDS_THREADS = (1 << 0)
} EvaMainLoopCreateFlags;

EvaMainLoop     *eva_main_loop_new       (EvaMainLoopCreateFlags create_flags);

/* return the per-thread main-loop */
EvaMainLoop     *eva_main_loop_default      (void) G_GNUC_CONST;


/* TIMEOUT is the maximum number of milliseconds to wait,
 * or pass in -1 to block forever.
 */
guint            eva_main_loop_run          (EvaMainLoop       *main_loop,
                                             gint               timeout,
                                             guint             *t_waited_out);
EvaSource       *eva_main_loop_add_idle     (EvaMainLoop       *main_loop,
                                             EvaMainLoopIdleFunc source_func,
                                             gpointer           user_data,
                                             GDestroyNotify     destroy);
EvaSource       *eva_main_loop_add_signal   (EvaMainLoop       *main_loop,
                                             int                signal_number,
                                             EvaMainLoopSignalFunc signal_func,
                                             gpointer           user_data,
                                             GDestroyNotify     destroy);
EvaSource       *eva_main_loop_add_waitpid  (EvaMainLoop       *main_loop,
                                             int                process_id,
                                           EvaMainLoopWaitPidFunc waitpid_func,
                                             gpointer           user_data,
                                             GDestroyNotify     destroy);
EvaSource       *eva_main_loop_add_io       (EvaMainLoop       *main_loop,
                                             int                fd,
                                             guint              events,
                                             EvaMainLoopIOFunc  io_func,
                                             gpointer           user_data,
                                             GDestroyNotify     destroy);
void             eva_source_adjust_io       (EvaSource         *source,
                                             guint              events);
void             eva_source_add_io_events   (EvaSource         *source,
                                             guint              events);
void             eva_source_remove_io_events(EvaSource         *source,
                                             guint              events);
#define eva_main_loop_add_timer eva_main_loop_add_timer64
#define eva_source_adjust_timer eva_source_adjust_timer64
EvaSource       *eva_main_loop_add_timer    (EvaMainLoop       *main_loop,
                                             EvaMainLoopTimeoutFunc timer_func,
                                             gpointer           timer_data,
                                             GDestroyNotify     timer_destroy,
                                             gint64             millis_expire,
                                             gint64             milli_period);
EvaSource       *eva_main_loop_add_timer_absolute
                                            (EvaMainLoop       *main_loop,
                                             EvaMainLoopTimeoutFunc timer_func,
                                             gpointer           timer_data,
                                             GDestroyNotify     timer_destroy,
                                             int                unixtime,
                                             int                unixtime_micro);
void             eva_source_adjust_timer    (EvaSource         *timer_source,
                                             gint64             millis_expire,
                                             gint64             milli_period);
void             eva_source_remove          (EvaSource         *source);
void             eva_main_loop_add_context  (EvaMainLoop       *main_loop,
					     GMainContext      *context);
void             eva_main_loop_quit         (EvaMainLoop       *main_loop);


gboolean         eva_main_loop_should_continue
                                            (EvaMainLoop       *main_loop);

EvaMainLoop *eva_source_peek_main_loop (EvaSource *source);

/*< protected >*/
void eva_main_loop_destroy_all_sources (EvaMainLoop *main_loop);

/* miscellaneous: should probably be private. */
gboolean eva_main_loop_do_waitpid (int                  pid,
                                   EvaMainLoopWaitInfo *wait_info);

/*< private >*/
void _eva_main_loop_init ();
void _eva_main_loop_fork_notify ();

/* for binary-compatibility, the library defines eva_main_loop_add_timer()
   with native-int timeouts.  but people compiling with the latest version
   will use eva_main_loop_add_timer64.  eventually, we can get rid of this hack,
   and just define eva_main_loop_add_timer() as the 64-bit function. */
#undef eva_main_loop_add_timer
#undef eva_source_adjust_timer
EvaSource *eva_main_loop_add_timer (EvaMainLoop*,EvaMainLoopTimeoutFunc,gpointer,GDestroyNotify,int,int);
void eva_source_adjust_timer (EvaSource*,int,int);
#define eva_main_loop_add_timer eva_main_loop_add_timer64
#define eva_source_adjust_timer eva_source_adjust_timer64

G_END_DECLS

#endif
