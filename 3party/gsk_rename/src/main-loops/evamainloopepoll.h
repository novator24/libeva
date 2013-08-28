#ifndef __EVA_MAIN_LOOP_EPOLL_H_
#define __EVA_MAIN_LOOP_EPOLL_H_

#include "evamainlooppollbase.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _EvaMainLoopEpoll EvaMainLoopEpoll;
typedef struct _EvaMainLoopEpollClass EvaMainLoopEpollClass;

/* --- type macros --- */
GType eva_main_loop_epoll_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_MAIN_LOOP_EPOLL			(eva_main_loop_epoll_get_type ())
#define EVA_MAIN_LOOP_EPOLL(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_MAIN_LOOP_EPOLL, EvaMainLoopEpoll))
#define EVA_MAIN_LOOP_EPOLL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_MAIN_LOOP_EPOLL, EvaMainLoopEpollClass))
#define EVA_MAIN_LOOP_EPOLL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_MAIN_LOOP_EPOLL, EvaMainLoopEpollClass))
#define EVA_IS_MAIN_LOOP_EPOLL(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_MAIN_LOOP_EPOLL))
#define EVA_IS_MAIN_LOOP_EPOLL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_MAIN_LOOP_EPOLL))

/* --- structures --- */
struct _EvaMainLoopEpollClass 
{
  EvaMainLoopPollBaseClass main_loop_poll_base_class;
};
struct _EvaMainLoopEpoll 
{
  EvaMainLoopPollBase      main_loop_poll_base;
  int                      fd;
  gpointer                 epoll_events;
};

/* --- prototypes --- */



G_END_DECLS

#endif
