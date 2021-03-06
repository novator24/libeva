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

#include "evamainloopkqueue.h"
#include "config.h"
static GObjectClass *parent_class = NULL;

#if HAVE_KQUEUE
#include <signal.h>
#include <errno.h>

/* Include headers for kqueue(), kevent() and struct kevent. */
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#endif

#include <unistd.h>    /* needed for close() */
#include <string.h>

/* --- EvaMainLoop methods --- */

static gboolean
eva_main_loop_kqueue_setup (EvaMainLoop *main_loop)
{
  EvaMainLoopKqueue *main_loop_kqueue = EVA_MAIN_LOOP_KQUEUE (main_loop);
#if HAVE_KQUEUE
  main_loop_kqueue->kernel_queue_id = kqueue ();
  return (main_loop_kqueue->kernel_queue_id >= 0);
#else
  (void) main_loop_kqueue;
  return FALSE;
#endif
}

#if HAVE_KQUEUE
static void
kqueue_flush_pending_changes (EvaMainLoopKqueue *main_loop_kqueue)
{
  if (main_loop_kqueue->num_updates > 0)
    {
      struct timespec timeout = { 0, 0 };
      kevent (main_loop_kqueue->kernel_queue_id,
              (const struct kevent *) main_loop_kqueue->kevent_array,
	      main_loop_kqueue->num_updates,
	      NULL, 0,
              &timeout);
      main_loop_kqueue->num_updates = 0;
    }
}

static inline void
change_signal (int signo, gboolean ignore)
{
  struct sigaction action;
  memset (&action, 0, sizeof (action));
  action.sa_handler = ignore ? SIG_IGN : SIG_DFL;
  sigaction (signo, &action, NULL);
}

static void do_nothing (int signo)
{
}

static inline void
almost_ignore_sigchld (void)
{
  struct sigaction action;
  memset (&action, 0, sizeof (action));
  action.sa_handler = do_nothing;
  sigaction (SIGCHLD, &action, NULL);
}

static inline struct kevent *
get_next_update ( EvaMainLoopKqueue *main_loop_kqueue)
{
  if (main_loop_kqueue->num_updates == main_loop_kqueue->max_updates)
    {
      if (main_loop_kqueue->max_updates == 0)
        main_loop_kqueue->max_updates = 128;
      else
        main_loop_kqueue->max_updates *= 2;
      main_loop_kqueue->kevent_array 
	= g_realloc (main_loop_kqueue->kevent_array,
		     sizeof (struct kevent) * main_loop_kqueue->max_updates);
    }
  return ((struct kevent *) (main_loop_kqueue->kevent_array))
        + main_loop_kqueue->num_updates;
}
  

static void 
eva_main_loop_kqueue_change (EvaMainLoop       *main_loop,
                             EvaMainLoopChange *change)
{
  EvaMainLoopKqueue *main_loop_kqueue = EVA_MAIN_LOOP_KQUEUE (main_loop);
  if (main_loop_kqueue->num_updates + 1 >= main_loop_kqueue->max_updates)
    {
      if (main_loop_kqueue->max_updates == 0)
        main_loop_kqueue->max_updates = 128;
      else
        main_loop_kqueue->max_updates *= 2;
      main_loop_kqueue->kevent_array 
	= g_realloc (main_loop_kqueue->kevent_array,
		     sizeof (struct kevent) * main_loop_kqueue->max_updates);
    }
  switch (change->type)
    {
      static gboolean has_ignored_sigchld = FALSE;
      struct kevent *event;
    case EVA_MAIN_LOOP_EVENT_PROCESS:
      if (change->data.process.did_exit)
        return;
      event = get_next_update (main_loop_kqueue);
      event->ident = change->data.process.pid;
      event->filter = EVFILT_PROC;
      event->flags = change->data.process.add ? (EV_ADD|EV_ONESHOT|EV_CLEAR) : EV_DELETE;
      event->fflags = NOTE_EXIT;
      event->data = 0;
      event->udata = NULL;
      main_loop_kqueue->num_updates++;
      if (!has_ignored_sigchld)
        {
          almost_ignore_sigchld ();
          has_ignored_sigchld = TRUE;
        }
      break;
    case EVA_MAIN_LOOP_EVENT_SIGNAL:
      event = get_next_update (main_loop_kqueue);
      event->ident = change->data.signal.number;
      event->filter = EVFILT_SIGNAL;
      event->flags = change->data.signal.add ? EV_ADD : EV_DELETE;
      event->fflags = 0;
      event->data = 0;
      event->udata = NULL;
      change_signal (change->data.signal.number, change->data.signal.add);
      main_loop_kqueue->num_updates++;
      kqueue_flush_pending_changes (main_loop_kqueue);
      break;
    case EVA_MAIN_LOOP_EVENT_IO:
      if ((change->data.io.old_events ^ change->data.io.events) & G_IO_IN)
	{
          event = get_next_update (main_loop_kqueue);
	  event->ident = change->data.io.fd;
	  event->filter = EVFILT_READ;
	  event->flags = (change->data.io.events & G_IO_IN) ? EV_ADD : EV_DELETE;
	  event->fflags = 0;
	  event->data = 0;
	  event->udata = NULL;
	  main_loop_kqueue->num_updates++;
	}
      if ((change->data.io.old_events ^ change->data.io.events) & G_IO_OUT)
	{
          event = get_next_update (main_loop_kqueue);
	  event->ident = change->data.io.fd;
	  event->filter = EVFILT_WRITE;
	  event->flags = (change->data.io.events & G_IO_OUT) ? EV_ADD : EV_DELETE;
	  event->fflags = 0;
	  event->data = 0;
	  event->udata = NULL;
	  main_loop_kqueue->num_updates++;
	}
      kqueue_flush_pending_changes (main_loop_kqueue);
      break;
    }
}

static guint
eva_main_loop_kqueue_poll (EvaMainLoop       *main_loop,
                           guint              max_events_out,
                           EvaMainLoopEvent  *events,
                           gint               timeout_milli)
{
  struct kevent *out = alloca (sizeof (struct kevent) * max_events_out);
  EvaMainLoopKqueue *main_loop_kqueue = EVA_MAIN_LOOP_KQUEUE (main_loop);
  gint kevent_rv;
  guint i;
  guint rv = 0;
  struct timespec timeout;
  struct timespec *p_timeout;
  if (timeout_milli < 0)
    {
      p_timeout = NULL;
    }
  else
    {
      timeout.tv_sec = timeout_milli / 1000;
      timeout.tv_nsec = timeout_milli % 1000 * 1000 * 1000;
      p_timeout = &timeout;
    }
retry_kevent:
  kevent_rv = kevent (main_loop_kqueue->kernel_queue_id,
		      main_loop_kqueue->kevent_array,
		      main_loop_kqueue->num_updates,
		      out,
		      max_events_out,
		      p_timeout);

  if (kevent_rv < 0)
    {
      if (eva_errno_is_ignorable (errno))
        goto retry_kevent;
      g_warning ("error running kevent: %s", g_strerror (errno));
      return 0;
    }
      
  main_loop_kqueue->num_updates = 0;

  for (i = 0; i < kevent_rv; i++)
    {
      switch (out[i].filter)
        {
        case EVFILT_READ:
          events[rv].type = EVA_MAIN_LOOP_EVENT_IO;
          events[rv].data.io.events = G_IO_IN;
          events[rv].data.io.fd = out[i].ident;
          rv++;
          break;
          
        case EVFILT_WRITE:
          events[rv].type = EVA_MAIN_LOOP_EVENT_IO;
          events[rv].data.io.events = G_IO_OUT;
          events[rv].data.io.fd = out[i].ident;
          rv++;
          break;

        case EVFILT_SIGNAL:
          events[rv].type = EVA_MAIN_LOOP_EVENT_SIGNAL;
          events[rv].data.signal = out[i].ident;
          rv++;
          break;

        case EVFILT_PROC:
          if (out[i].fflags == NOTE_EXIT)
            {
              events[rv].type = EVA_MAIN_LOOP_EVENT_PROCESS;
              events[rv].data.process_wait_info.pid = out[i].ident;
              if (eva_main_loop_do_waitpid (out[i].ident, 
                                         &events[rv].data.process_wait_info))
                rv++;
            }
          break;

        default:
          g_warning ("unexpected type of event from kevent (%d)",
                     out[i].filter);
        }
    }
  return rv;
}
#endif

static void    
eva_main_loop_kqueue_finalize(GObject *object)
{
  EvaMainLoopKqueue *kqueue = EVA_MAIN_LOOP_KQUEUE (object);
  eva_main_loop_destroy_all_sources (EVA_MAIN_LOOP (object));
  if (kqueue->kernel_queue_id >= 0)
    close (kqueue->kernel_queue_id);
  (*parent_class->finalize) (object);
}

/* --- class methods --- */
static void
eva_main_loop_kqueue_init (EvaMainLoopKqueue *kqueue)
{
  kqueue->kernel_queue_id = -1;
}

static void
eva_main_loop_kqueue_class_init (EvaMainLoopClass *main_loop_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (main_loop_class);
  parent_class = g_type_class_peek_parent (main_loop_class);
  main_loop_class->setup = eva_main_loop_kqueue_setup;
#if HAVE_KQUEUE
  main_loop_class->poll = eva_main_loop_kqueue_poll;
  main_loop_class->change = eva_main_loop_kqueue_change;
#endif
  object_class->finalize = eva_main_loop_kqueue_finalize;
}

GType
eva_main_loop_kqueue_get_type()
{
  static GType main_loop_kqueue_type = 0;
  if (!main_loop_kqueue_type)
    {
      static const GTypeInfo main_loop_kqueue_info =
      {
	sizeof(EvaMainLoopKqueueClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) eva_main_loop_kqueue_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (EvaMainLoopKqueue),
	0,		/* n_preallocs */
	(GInstanceInitFunc) eva_main_loop_kqueue_init,
	NULL		/* value_table */
      };
      GType parent = EVA_TYPE_MAIN_LOOP;
      main_loop_kqueue_type = g_type_register_static (parent,
                                                  "EvaMainLoopKqueue",
						  &main_loop_kqueue_info, 0);
    }
  return main_loop_kqueue_type;
}
