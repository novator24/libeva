#ifndef __EVA_MAIN_LOOP_DEV_POLL_H_
#define __EVA_MAIN_LOOP_DEV_POLL_H_

#include "evamainlooppollbase.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _GskMainLoopDevPoll GskMainLoopDevPoll;
typedef struct _GskMainLoopDevPollClass GskMainLoopDevPollClass;

/* --- type macros --- */
GType eva_main_loop_dev_poll_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_MAIN_LOOP_DEV_POLL			(eva_main_loop_dev_poll_get_type ())
#define EVA_MAIN_LOOP_DEV_POLL(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_MAIN_LOOP_DEV_POLL, GskMainLoopDevPoll))
#define EVA_MAIN_LOOP_DEV_POLL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_MAIN_LOOP_DEV_POLL, GskMainLoopDevPollClass))
#define EVA_MAIN_LOOP_DEV_POLL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_MAIN_LOOP_DEV_POLL, GskMainLoopDevPollClass))
#define EVA_IS_MAIN_LOOP_DEV_POLL(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_MAIN_LOOP_DEV_POLL))
#define EVA_IS_MAIN_LOOP_DEV_POLL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_MAIN_LOOP_DEV_POLL))

/* --- structures --- */
struct _GskMainLoopDevPollClass 
{
  GskMainLoopPollBaseClass	main_loop_class;
};
struct _GskMainLoopDevPoll 
{
  GskMainLoopPollBase		main_loop;
  int                   	dev_poll_fd;
};


G_END_DECLS

#endif
