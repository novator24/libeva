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

#ifndef __EVA_MAIN_LOOP_KQUEUE_H_
#define __EVA_MAIN_LOOP_KQUEUE_H_

#include "../evamainloop.h"

G_BEGIN_DECLS

typedef struct _EvaMainLoopKqueue EvaMainLoopKqueue;
typedef struct _EvaMainLoopKqueueClass EvaMainLoopKqueueClass;

/* --- type macros --- */
GType eva_main_loop_kqueue_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_MAIN_LOOP_KQUEUE			(eva_main_loop_kqueue_get_type ())
#define EVA_MAIN_LOOP_KQUEUE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_MAIN_LOOP_KQUEUE, EvaMainLoopKqueue))
#define EVA_MAIN_LOOP_KQUEUE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_MAIN_LOOP_KQUEUE, EvaMainLoopKqueueClass))
#define EVA_MAIN_LOOP_KQUEUE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_MAIN_LOOP_KQUEUE, EvaMainLoopKqueueClass))
#define EVA_IS_MAIN_LOOP_KQUEUE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_MAIN_LOOP_KQUEUE))
#define EVA_IS_MAIN_LOOP_KQUEUE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_MAIN_LOOP_KQUEUE))


struct _EvaMainLoopKqueueClass
{
  EvaMainLoopClass	main_loop_class;
};

struct _EvaMainLoopKqueue
{
  EvaMainLoop		main_loop;

  guint                 num_updates;
  guint                 max_updates;
  gpointer              kevent_array;

  /* this may be used by init_polling / do_polling as the class desires,
   * except you must reset it to -1 before finalizing, or else
   * we will close(2) it.
   */
  int			kernel_queue_id;
};

G_END_DECLS

#endif
