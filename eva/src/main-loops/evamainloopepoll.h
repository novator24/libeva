#ifndef __EVA_MAIN_LOOP_EPOLL_H_
#define __EVA_MAIN_LOOP_EPOLL_H_

#include "evamainlooppollbase.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _GskMainLoopEpoll GskMainLoopEpoll;
typedef struct _GskMainLoopEpollClass GskMainLoopEpollClass;

/* --- type macros --- */
GType eva_main_loop_epoll_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_MAIN_LOOP_EPOLL			(eva_main_loop_epoll_get_type ())
#define EVA_MAIN_LOOP_EPOLL(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_MAIN_LOOP_EPOLL, GskMainLoopEpoll))
#define EVA_MAIN_LOOP_EPOLL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_MAIN_LOOP_EPOLL, GskMainLoopEpollClass))
#define EVA_MAIN_LOOP_EPOLL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_MAIN_LOOP_EPOLL, GskMainLoopEpollClass))
#define EVA_IS_MAIN_LOOP_EPOLL(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_MAIN_LOOP_EPOLL))
#define EVA_IS_MAIN_LOOP_EPOLL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_MAIN_LOOP_EPOLL))

/* --- structures --- */
struct _GskMainLoopEpollClass 
{
  GskMainLoopPollBaseClass main_loop_poll_base_class;
};
struct _GskMainLoopEpoll 
{
  GskMainLoopPollBase      main_loop_poll_base;
  int                      fd;
  gpointer                 epoll_events;
};

/* --- prototypes --- */



G_END_DECLS

#endif
