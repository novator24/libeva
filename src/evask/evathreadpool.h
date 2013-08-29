#ifndef __EVA_THREAD_POOL_H_
#define __EVA_THREAD_POOL_H_

#include <eva/evamainloop.h>

G_BEGIN_DECLS

typedef struct _EvaThreadPool EvaThreadPool;

typedef gpointer (*EvaThreadPoolRunFunc)    (gpointer  run_data);
typedef void     (*EvaThreadPoolResultFunc) (gpointer  run_data,
                                             gpointer  result_data);
typedef void     (*EvaThreadPoolDestroyFunc)(gpointer  run_data,
                                             gpointer  result_data);

EvaThreadPool *eva_thread_pool_new     (EvaMainLoop             *main_loop,
                                        guint                    max_threads);
void           eva_thread_pool_push    (EvaThreadPool           *pool,
                                        EvaThreadPoolRunFunc     run,
			                EvaThreadPoolResultFunc  handle_result,
                                        gpointer                 run_data,
			                EvaThreadPoolDestroyFunc destroy);
void           eva_thread_pool_destroy (EvaThreadPool           *pool,
					GDestroyNotify           destroy,
					gpointer                 destroy_data);

G_END_DECLS

#endif
