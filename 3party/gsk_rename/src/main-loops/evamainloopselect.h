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
/* EvaMainLoopSelect: A main loop based around the select(2) system call. */

/* Notes:
 * 
            !!! UNTESTED !!!

 * even if it were tested, use would be discouraged in favor of
 * the poll(2) implementation.
 */

#ifndef __EVA_MAIN_LOOP_SELECT_H_
#define __EVA_MAIN_LOOP_SELECT_H_

/* for fd_set */
#include <sys/time.h>
#include <sys/types.h>

#include "evamainlooppollbase.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _EvaMainLoopSelect EvaMainLoopSelect;
typedef struct _EvaMainLoopSelectClass EvaMainLoopSelectClass;


/* --- type macros --- */
GType eva_main_loop_select_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_MAIN_LOOP_SELECT			(eva_main_loop_select_get_type ())
#define EVA_MAIN_LOOP_SELECT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_MAIN_LOOP_SELECT, EvaMainLoopSelect))
#define EVA_MAIN_LOOP_SELECT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_MAIN_LOOP_SELECT, EvaMainLoopSelectClass))
#define EVA_MAIN_LOOP_SELECT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_MAIN_LOOP_SELECT, EvaMainLoopSelectClass))
#define EVA_IS_MAIN_LOOP_SELECT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_MAIN_LOOP_SELECT))
#define EVA_IS_MAIN_LOOP_SELECT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_MAIN_LOOP_SELECT))


/* --- structures --- */
struct _EvaMainLoopSelectClass 
{
  EvaMainLoopPollBaseClass	main_loop_poll_base_class;
};
struct _EvaMainLoopSelect 
{
  EvaMainLoopPollBase		main_loop_poll_base;
  GTree                        *fd_tree;
  fd_set			read_set;
  fd_set			write_set;
  fd_set                        except_set;
};

G_END_DECLS

#endif
