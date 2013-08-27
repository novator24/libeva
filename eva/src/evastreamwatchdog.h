/*
    EVA - a library to write servers

    Copyright (C) 2006 Dave Benson

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

#ifndef __EVA_STREAM_WATCHDOG_H_
#define __EVA_STREAM_WATCHDOG_H_

typedef struct _EvaStreamWatchdogClass EvaStreamWatchdogClass;
typedef struct _EvaStreamWatchdog EvaStreamWatchdog;

#include "evastream.h"
#include "evamainloop.h"

GType eva_stream_watchdog_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_STREAM_WATCHDOG              (eva_stream_watchdog_get_type ())
#define EVA_STREAM_WATCHDOG(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_STREAM_WATCHDOG, EvaStreamWatchdog))
#define EVA_STREAM_WATCHDOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_STREAM_WATCHDOG, EvaStreamWatchdogClass))
#define EVA_STREAM_WATCHDOG_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_STREAM_WATCHDOG, EvaStreamWatchdogClass))
#define EVA_IS_STREAM_WATCHDOG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_STREAM_WATCHDOG))
#define EVA_IS_STREAM_WATCHDOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_STREAM_WATCHDOG))

struct _EvaStreamWatchdogClass
{
  EvaStreamClass base_class;
};
struct _EvaStreamWatchdog
{
  EvaStream base_instance;
  EvaStream *underlying;
  EvaSource *timeout;
  guint max_inactivity_millis;
};

EvaStream *eva_stream_watchdog_new (EvaStream       *underlying_stream,
                                    guint            max_inactivity_millis);

#endif
