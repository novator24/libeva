#include "evahook.h"
#include "evatree.h"
#include "evamainloop.h"
#include "evadebug.h"
#include "debug.h"

/* privately used flags */
enum
{
  EVA_HOOK_HAS_POLL		          = (1 << 8),
  EVA_HOOK_IS_NOTIFYING 	          = (1 << 9),
  EVA_HOOK_IS_NOTIFYING_SHUTDOWN          = (1 << 10),
  EVA_HOOK_BLOCKED_NOTIFY                 = (1 << 11),
  EVA_HOOK_BLOCKED_SHUTDOWN_NOTIFY        = (1 << 12),
  EVA_HOOK_UNTRAPPED_DURING_NOTIFY        = (1 << 13),

  /* aliases */
  EVA_HOOK_IDLE_NOTIFY		         = EVA_HOOK_private_IDLE_NOTIFY,
  EVA_HOOK_JUST_NEVER_BLOCKS	         = EVA_HOOK_private_JUST_NEVER_BLOCKS,
  EVA_HOOK_NEVER_BLOCKS                  = EVA_HOOK_private_NEVER_BLOCKS,
  EVA_HOOK_CAN_DEFER_SHUTDOWN            = EVA_HOOK_private_CAN_DEFER_SHUTDOWN,
  EVA_HOOK_SHUTTING_DOWN                 = EVA_HOOK_private_SHUTTING_DOWN
};

static GQuark eva_hook_main_loop_quark = 0;

#ifdef EVA_DEBUG
static void
eva_hook_debug (EvaHook    *hook,
		const char *format,
		...) G_GNUC_PRINTF(2,3);
#define DEBUG(args)			\
	G_STMT_START{ 			\
	  if (EVA_IS_DEBUGGING (HOOK)) 	\
	    eva_hook_debug args;	\
        }G_STMT_END
#else
#define DEBUG(args)
#endif

extern EvaDebugFlags eva_debug_flags;

/* --- implementation of non-blocking hooks --- */
typedef struct _PendingDestroyNotify PendingDestroyNotify;
struct _PendingDestroyNotify
{
  /* WARNING: this structure must be no more than 3 pointers to work! */
  gpointer data;
  GDestroyNotify destroy;
  PendingDestroyNotify *next;
};
/* abuse GList allocator */
#define pending_destroy_notify_alloc() (PendingDestroyNotify*)g_list_alloc()
#define pending_destroy_notify_free(notify) g_list_free_1((GList*)notify)

typedef struct 
{
  EvaTree *hook_tree;
  EvaSource *source;
  PendingDestroyNotify *first_destroy;
  PendingDestroyNotify *last_destroy;
} NBThreadData;

static void
flush_pending_destroys (NBThreadData *nb_data)
{
  while (nb_data->first_destroy)
    {
      PendingDestroyNotify *at = nb_data->first_destroy;
      nb_data->first_destroy = at->next;
      if (nb_data->first_destroy == NULL)
        nb_data->last_destroy = NULL;
      at->destroy (at->data);
      pending_destroy_notify_free (at);
    }
}

static gboolean
run_all_nonblocking_hooks (gpointer data)
{
  NBThreadData *nb_data = data;
  EvaTree *tree = nb_data->hook_tree;
  EvaTreeNode *node;

  flush_pending_destroys (nb_data);

  node = eva_tree_node_first (tree);
  if (EVA_IS_DEBUGGING (HOOK))
    g_message ("run_all_nonblocking_hooks: %d hooks, first=%p", eva_tree_n_nodes (tree), node);

  /* is the nonblocking thread totally inactive?   if so, remove it. */
  if (node == NULL)
    {
      nb_data->source = NULL;
      return FALSE;
    }
  do
    {
      EvaHook *hook = eva_tree_node_peek_key (node);
      DEBUG((hook, "about to run notify, since this hook is nonblocking"));
      eva_hook_notify (hook);
      node = eva_tree_node_next (tree, node);
    }
  while (node != NULL);

  flush_pending_destroys (nb_data);
  if (EVA_IS_DEBUGGING (HOOK))
    g_message ("done -- run_all_nonblocking_hooks");
  return TRUE;
}

static void
destroy_nonblocking_thread_data (gpointer data)
{
  NBThreadData *nb_data = data;
  /* ignore source -- it's going away with the main-loop */
  eva_tree_unref (nb_data->hook_tree);
  g_free (nb_data);
}

static gint
pointer_compare (gconstpointer a, gconstpointer b)
{
  const char *pa = a;
  const char *pb = b;
  return (pa < pb) ? -1 : (pa > pb) ? +1 : 0;
}

static inline NBThreadData *
main_loop_force_nonblocking_data (EvaMainLoop *loop)
{
  NBThreadData *data = g_object_get_qdata (G_OBJECT (loop), eva_hook_main_loop_quark);
  if (data == NULL)
    {
      data = g_new (NBThreadData, 1);
      data->hook_tree = eva_tree_new (pointer_compare);
      data->source = NULL;
      data->first_destroy = data->last_destroy = NULL;
      g_object_set_qdata_full (G_OBJECT (loop), eva_hook_main_loop_quark, data,
			       destroy_nonblocking_thread_data);
    }
  return data;
}

static inline void
verify_nonblocking_data_has_source (NBThreadData *nb_data,
			            EvaMainLoop  *main_loop)
{
  if (nb_data->source == NULL)
    nb_data->source = eva_main_loop_add_idle (main_loop,
					      run_all_nonblocking_hooks,
					      nb_data, NULL);
}

static inline void
eva_hook_add_to_nonblocking_list (EvaHook *hook)
{
  EvaMainLoop *loop = eva_main_loop_default ();
  NBThreadData *data = main_loop_force_nonblocking_data (loop);
  eva_tree_insert (data->hook_tree, hook, hook);
  verify_nonblocking_data_has_source (data, loop);
}

static inline void
eva_hook_remove_from_nonblocking_list (EvaHook *hook)
{
  EvaMainLoop *loop = eva_main_loop_default ();
  NBThreadData *data = main_loop_force_nonblocking_data (loop);
  eva_tree_remove (data->hook_tree, hook);

  /* note: to avoid deleting and re-adding the idle function repeatedly
   *       in certain cases, we always do the removal by returning FALSE
   *       from the run_all_nonblocking_hooks
   */
}

static inline void
eva_hook_maybe_defer_destroy (GDestroyNotify destroy, gpointer destroy_data)
{
  if (destroy)
    {
      EvaMainLoop *loop = eva_main_loop_default ();
      NBThreadData *data = main_loop_force_nonblocking_data (loop);
      PendingDestroyNotify *notify = pending_destroy_notify_alloc ();
      verify_nonblocking_data_has_source (data, loop);
      notify->destroy = destroy;
      notify->data = destroy_data;
      notify->next = NULL;
      if (data->last_destroy)
        data->last_destroy->next = notify;
      else
        data->first_destroy = notify;
      data->last_destroy = notify;
    }
}

/* --- public interface --- */

/** 
  * eva_hook_init:
  * @hook: The hook to initialize.
  * @flags: Special details about the hook.
  * @inset: Offset in bytes of this hook inside the containing object.
  * @class_set_poll_offset: Offset in bytes of the set_poll
  * method in the containing object's class.
  * @class_shutdown_offset: Offset in bytes of the shutdown
  * method in the containing object's class, or 0 if there is no shutdown
  * method.
  *
  * Prepare a EvaHook to be used.  This should almost always
  * be done in the instance-init function of the class which contains the hook.
  */
void
eva_hook_init           (EvaHook        *hook,
			 EvaHookFlags    flags,
			 guint           inset,
			 guint           class_set_poll_offset,
			 guint           class_shutdown_offset)
{
  /* currently these are stored in guint16's... */
  g_return_if_fail (inset < 65536
		 && class_shutdown_offset < 65536
		 && class_set_poll_offset < 65536);

  hook->flags = flags & ~_EVA_HOOK_FLAGS_RESERVED;
  hook->block_count = 0;
  hook->inset = inset;
  hook->class_set_poll_offset = class_set_poll_offset;
  hook->class_shutdown_offset = class_shutdown_offset;
  hook->func = NULL;
  hook->shutdown_func = NULL;
  hook->data = NULL;
  hook->destroy = NULL;
}

static inline void
eva_hook_call_set_poll_func (EvaHook *hook,
		             gboolean value)
{
  GObject *object = EVA_HOOK_GET_OBJECT (hook);
  GObjectClass *class = G_OBJECT_GET_CLASS (object);
  EvaHookSetPollFunc set_poll_func =
    G_STRUCT_MEMBER (EvaHookSetPollFunc, class, hook->class_set_poll_offset);
  if (set_poll_func)
    (*set_poll_func) (object, value);
}

static inline gboolean
eva_hook_call_shutdown_func (EvaHook *hook,
			     GError **error)
{
  GObject *object = EVA_HOOK_GET_OBJECT (hook);
  GObjectClass *class = G_OBJECT_GET_CLASS (object);
  guint offset = hook->class_shutdown_offset;
  GHookFunc shutdown_func =
    (GHookFunc) G_STRUCT_MEMBER (GHookFunc, class, offset);
  if (!shutdown_func)
    return TRUE;
  if (EVA_HOOK_TEST_FLAG (hook, CAN_HAVE_SHUTDOWN_ERROR))
    return (* (EvaHookShutdownErrorFunc) shutdown_func) (object, error);
  else
    {
      (* (EvaHookShutdownFunc) shutdown_func) (object);
      return TRUE;
    }
}

static inline void
eva_hook_set_poll (EvaHook *hook)
{
  DEBUG ((hook, "eva_hook_set_poll: idle-notify=%d",
	  EVA_HOOK_TEST_FLAG (hook, IDLE_NOTIFY)));

  EVA_HOOK_SET_FLAG (hook, HAS_POLL);
  if (EVA_HOOK_TEST_FLAG (hook, IDLE_NOTIFY))
    eva_hook_add_to_nonblocking_list (hook);
  eva_hook_call_set_poll_func (hook, TRUE);
}

static inline void
eva_hook_clear_poll (EvaHook *hook)
{
  DEBUG ((hook, "eva_hook_clear_poll: idle-notify=%d",
	  EVA_HOOK_TEST_FLAG (hook, IDLE_NOTIFY)));

  EVA_HOOK_CLEAR_FLAG (hook, HAS_POLL);
  if (EVA_HOOK_TEST_FLAG (hook, IDLE_NOTIFY))
    eva_hook_remove_from_nonblocking_list (hook);
  eva_hook_call_set_poll_func (hook, FALSE);
}

/**
 * eva_hook_trap:
 * @hook: the hook to trap
 * @func: function to call with the hook is triggered.
 * @shutdown: function to be called if the hook is shutting down and will
 * never trigger again.
 * @data: user-data to pass to func and shutdown.
 * @destroy: function to call on user-data when the hook is untrapped.
 *
 * Traps hook triggers and shutdowns.  The caller's functions will
 * be run in response to #eva_hook_notify.
 */
void
eva_hook_trap           (EvaHook        *hook,
			 EvaHookFunc     func,
			 EvaHookFunc     shutdown,
			 gpointer        data,
			 GDestroyNotify  destroy)
{
  g_return_if_fail (hook->func == NULL);
  g_return_if_fail (EVA_HOOK_TEST_FLAG (hook, IS_AVAILABLE));
  DEBUG ((hook, "eva_hook_trap"));
  hook->func = func;
  hook->shutdown_func = shutdown;
  hook->data = data;
  hook->destroy = destroy;
  if (hook->block_count == 0
   && !EVA_HOOK_TEST_FLAG (hook, HAS_POLL))
    eva_hook_set_poll (hook);
}

/**
 * eva_hook_untrap:
 * @hook: the hook to untrap
 *
 * Stops trapping the hook.  The user's function will
 * no longer be run, and invoke the user's destroy method,
 * if it was supplied to #eva_hook_trap.
 *
 * If you untrap the hook while it is executing,
 * the destroy handler will be deferred
 * until after the hook is done notifying and/or shutdown-notifying.
 */
void
eva_hook_untrap          (EvaHook        *hook)
{
  GDestroyNotify old_destroy = hook->destroy;
  gpointer old_data = hook->data;

  DEBUG ((hook, "eva_hook_untrap"));

  hook->func = NULL;
  hook->shutdown_func = NULL;
  hook->data = NULL;
  hook->destroy = NULL;

  if (EVA_HOOK_TEST_FLAG (hook, HAS_POLL))
    eva_hook_clear_poll (hook);

  /* If a hook is untrapped while one of the user's callbacks is running,
     eva_hook_notify and eva_hook_notify_shutdown need to ignore the
     callback's return value. */
  if (EVA_HOOK_TEST_FLAG (hook, IS_NOTIFYING) 
   || EVA_HOOK_TEST_FLAG (hook, IS_NOTIFYING_SHUTDOWN))
    {
      EVA_HOOK_SET_FLAG (hook, UNTRAPPED_DURING_NOTIFY);
      eva_hook_maybe_defer_destroy (old_destroy, old_data);
    }
  else if (old_destroy)
    old_destroy (old_data);
}

/**
 * eva_hook_block:
 * @hook: the hook to block.
 *
 * Temporarily block the hook from triggerring the callback.
 * This may cause the set_poll method to be run.
 *
 * This increases a block_count, so you may call
 * eva_hook_block as many times as you like,
 * but you must call #eva_hook_unblock an equal number of times.
 */
void
eva_hook_block           (EvaHook        *hook)
{
  hook->block_count++;
  DEBUG ((hook, "eva_hook_block: new-count=%d", hook->block_count));
  if (EVA_HOOK_TEST_FLAG (hook, HAS_POLL))
    eva_hook_clear_poll (hook);
}

/**
 * eva_hook_unblock:
 * @hook: the hook to unblock.
 *
 * Undoes a eva_hook_block().  The block count is decreased,
 * and if it gets to 0, the hook may be eligible to start
 * triggering again.
 */
void
eva_hook_unblock         (EvaHook        *hook)
{
  g_return_if_fail (hook->block_count > 0);
  hook->block_count--;
  DEBUG ((hook, "eva_hook_unblock: new-count=%d", hook->block_count));
  if (hook->block_count == 0
   && EVA_HOOK_TEST_FLAG (hook, IS_AVAILABLE)
   && hook->func != NULL)
    eva_hook_set_poll (hook);
}

/**
 * eva_hook_shutdown:
 * @hook: the hook to shut down.
 * @error: optional error return.
 *
 * Shutdown a hook.  You may call eva_hook_shutdown on a hook more than
 * once, but it will be ignored after the first time.
 *
 * Note that when eva_hook_shutdown succeeds, the hook is not necessarily
 * completely shut down.  Some hooks (for example, for a socket waiting
 * for a reply from a remote server) may remain in the SHUTTING_DOWN state
 * for some time before the shutdown completes and the user's shutdown
 * callback is invoked.  You may test whether the shutdown has completed
 * using EVA_HOOK_TEST_SHUTTING_DOWN (hook): if true, the shutdown is still
 * in progress.
 *
 * Regardless of whether the shutdown succeeds, or whether the shutdown
 * completes immediately, the hook will be marked unavailable after
 * calling eva_hook_shutdown.  (That is, EVA_HOOK_TEST_IS_AVAILABLE (hook)
 * will be false.)
 *
 * returns: true if successful, or false if an error occurred.
 */
gboolean
eva_hook_shutdown        (EvaHook        *hook,
			  GError        **error)
{
  GObject *object = EVA_HOOK_GET_OBJECT (hook);
  GError *dummy = NULL;
  GError **err = error ? error : &dummy;
  gboolean success, do_notify;

  DEBUG ((hook, "eva_hook_shutdown: is-available-initially=%d",
	  EVA_HOOK_TEST_FLAG (hook, IS_AVAILABLE)));

  if (!EVA_HOOK_TEST_FLAG (hook, IS_AVAILABLE))
    return TRUE;

  if (EVA_HOOK_TEST_FLAG (hook, SHUTTING_DOWN))
    return TRUE;

  g_object_ref (object);
  EVA_HOOK_SET_FLAG (hook, SHUTTING_DOWN);
  if (EVA_HOOK_TEST_FLAG (hook, CAN_DEFER_SHUTDOWN))
    {
      /* If the hook CAN_DEFER_SHUTDOWN, false indicates that the
	 shutdown was deferred, whether or not an error occurred. */
      if (eva_hook_call_shutdown_func (hook, err))
	do_notify = TRUE;
      else
	{
	  do_notify = FALSE;
	  /* The class method should not have cleared this, since it
	     wanted to defer shutdown. */
	  g_return_val_if_fail (EVA_HOOK_TEST_FLAG (hook, SHUTTING_DOWN),
				FALSE);
	}
      success = (*err == NULL);
    }
  else
    {
      /* If the hook can't defer shutdown, false indicates that
	 an error occurred. */
      if (eva_hook_call_shutdown_func (hook, err))
	success = (*err == NULL); /* TODO: ??? */
      else
	success = FALSE;

      do_notify = TRUE;
    }
  if (do_notify)
    {
      eva_hook_notify_shutdown (hook);

      /* We should be completely shut down, unless we were already
	 notifying and shutdown notification was blocked. */
      g_return_val_if_fail
	(!EVA_HOOK_TEST_FLAG (hook, SHUTTING_DOWN)
	 || (EVA_HOOK_TEST_FLAG (hook, IS_NOTIFYING)
	     && EVA_HOOK_TEST_FLAG (hook, BLOCKED_SHUTDOWN_NOTIFY)),
	 FALSE);
    }
  /* We always clear this here, in case shutdown notification was blocked. */
  EVA_HOOK_CLEAR_FLAG (hook, IS_AVAILABLE);

  if (dummy)
    g_error_free (dummy);

  g_object_unref (object);
  return success;
}

/* for use by classes which contain hooks */
/**
 * eva_hook_notify:
 * @hook: the hook which is triggering.
 *
 * This is called by the implementation of a class
 * which contains a hook to trigger it, i.e. call the user's
 * callback.
 * 
 * Generally, you should only call this if the user is ready
 * for data (so that set_poll has been called with TRUE);
 * however, if you call it, it will be remembered,
 * and immediately triggered once it is allowed to:
 * for example, when the block count gets to 0.
 *
 * Some gory details come up now and then:
 *
 * - there are reentrance guards which prevent recursive calls
 *   to do anything.  Also, notify within shutdown is not allowed.
 */
void
eva_hook_notify          (EvaHook        *hook)
{
  GObject *object;
  gboolean handler_rv;

  g_return_if_fail (EVA_HOOK_TEST_FLAG (hook, IS_AVAILABLE));

  DEBUG ((hook, "eva_hook_notify: block-cnt=%d; notifying=%d, notifying-shutdown=%d, shutting-down=%d",
  	 hook->block_count,
	 EVA_HOOK_TEST_FLAG (hook, IS_NOTIFYING),
	 EVA_HOOK_TEST_FLAG (hook, IS_NOTIFYING_SHUTDOWN),
	 EVA_HOOK_TEST_FLAG (hook, SHUTTING_DOWN)));

  if (hook->block_count > 0
   || EVA_HOOK_TEST_FLAG (hook, IS_NOTIFYING)
   || EVA_HOOK_TEST_FLAG (hook, IS_NOTIFYING_SHUTDOWN))
    {
      EVA_HOOK_SET_FLAG (hook, BLOCKED_NOTIFY);
      return;
    }

  EVA_HOOK_CLEAR_FLAG (hook, BLOCKED_NOTIFY);
  
  object = EVA_HOOK_GET_OBJECT (hook);
  g_object_ref (object);

begin_notify:
  if (hook->func == NULL)
    {
      goto done_notify;
    }

  EVA_HOOK_SET_FLAG (hook, IS_NOTIFYING);
  handler_rv = hook->func (object, hook->data);
  EVA_HOOK_CLEAR_FLAG (hook, IS_NOTIFYING);

  if (!handler_rv
   && !EVA_HOOK_TEST_FLAG (hook, UNTRAPPED_DURING_NOTIFY))
    {
      eva_hook_untrap (hook);
    }
  EVA_HOOK_CLEAR_FLAG (hook, UNTRAPPED_DURING_NOTIFY);

  if (EVA_HOOK_TEST_FLAG (hook, BLOCKED_SHUTDOWN_NOTIFY))
    goto got_shutdown_notify;
  if (EVA_HOOK_TEST_FLAG (hook, BLOCKED_NOTIFY))
    {
      /* restart */
      EVA_HOOK_CLEAR_FLAG (hook, BLOCKED_NOTIFY);
      goto begin_notify;
    }

done_notify:
  g_object_unref (object);
  return;

got_shutdown_notify:
  eva_hook_notify_shutdown (hook);
  g_object_unref (object);
  return;
}

static inline void
_eva_hook_clear_idle_notify (EvaHook        *hook)
{
  EVA_HOOK_CLEAR_FLAG (hook, IDLE_NOTIFY);
  if (EVA_HOOK_TEST_FLAG (hook, HAS_POLL))
    eva_hook_remove_from_nonblocking_list (hook);
}

/**
 * eva_hook_notify_shutdown:
 * @hook: the hook which has shut down.
 *
 * Notify the user that a shutdown event has occurred.
 * This may happen because you called eva_hook_shutdown()
 * or because the hook was shut down by the object:
 * for example, the remote side of a connection may have shutdown.
 *
 * This does nothing if the hook is not presently available or in the
 * middle of shutting down (i.e., IS_AVAILABLE || SHUTTING_DOWN); after
 * this call the hook will be marked both unavailable and completely
 * shut down (i.e., !IS_AVAILABLE && !SHUTTING_DOWN).
 *
 * A shutdown hook will no longer be idle-notified.
 */
void
eva_hook_notify_shutdown (EvaHook        *hook)
{
  DEBUG ((hook, "eva_hook_notify_shutdown: available=%d, shutting-down=%d, notifying=%d, notifying-shutdown=%d",
	 EVA_HOOK_TEST_FLAG (hook, IS_AVAILABLE),
	 EVA_HOOK_TEST_FLAG (hook, SHUTTING_DOWN),
	 EVA_HOOK_TEST_FLAG (hook, IS_NOTIFYING),
	 EVA_HOOK_TEST_FLAG (hook, IS_NOTIFYING_SHUTDOWN)));

  /* Notify-shutdown within notify-shutdown is not allowed. */
  if (EVA_HOOK_TEST_FLAG (hook, IS_NOTIFYING_SHUTDOWN))
    return;

  /* If the hook is neither available nor shutting down, either the user has
     already been notified, or the hook was never available to begin with. */
  if (!EVA_HOOK_TEST_FLAG (hook, IS_AVAILABLE)
   && !EVA_HOOK_TEST_FLAG (hook, SHUTTING_DOWN))
    return;

  /* Notify-shutdown within notify will be deferred until the notification
     is complete. */
  if (EVA_HOOK_TEST_FLAG (hook, IS_NOTIFYING))
    {
      EVA_HOOK_SET_FLAG (hook, BLOCKED_SHUTDOWN_NOTIFY);
      return;
    }

  /* The hook is now completely shut down. */
  if (EVA_HOOK_TEST_FLAG (hook, IDLE_NOTIFY))
    _eva_hook_clear_idle_notify (hook);
  EVA_HOOK_CLEAR_FLAG (hook, IS_AVAILABLE);
  EVA_HOOK_CLEAR_FLAG (hook, SHUTTING_DOWN);

  /* Notify the user that the hook is completely shut down. */
  if (hook->shutdown_func == NULL)
    {
      if (eva_hook_is_trapped (hook))
	eva_hook_untrap (hook);
    }
  else
    {
      GObject *object = EVA_HOOK_GET_OBJECT (hook);
      gboolean handler_rv;
      g_object_ref (object);

      EVA_HOOK_SET_FLAG (hook, IS_NOTIFYING_SHUTDOWN);
      handler_rv = (*hook->shutdown_func) (object, hook->data);
      EVA_HOOK_CLEAR_FLAG (hook, IS_NOTIFYING_SHUTDOWN);

      if (!handler_rv
       && !EVA_HOOK_TEST_FLAG (hook, UNTRAPPED_DURING_NOTIFY))
	{
	  eva_hook_untrap (hook);
	}
      EVA_HOOK_CLEAR_FLAG (hook, UNTRAPPED_DURING_NOTIFY);
      g_object_unref (object);
    }
}

/* an alternative to implementing set_poll */

/**
 * eva_hook_set_idle_notify:
 * @hook: the hook to run constantly.
 * @should_idle_notify: whether to run the user's callback
 * continually.
 *
 * When idle_notify is set, the hook will run every cycle of the
 * main-loop.  Nothing will happen unless the user
 * has trapped the event, and it's not blocked.
 *
 * Opposite of eva_hook_clear_idle_notify().
 */
void
eva_hook_set_idle_notify   (EvaHook        *hook,
			    gboolean        should_idle_notify)
{
  /* TODO: optimize */
  if (should_idle_notify)
    eva_hook_mark_idle_notify (hook);
  else
    eva_hook_clear_idle_notify (hook);
}

/**
 * eva_hook_mark_idle_notify:
 * @hook: the hook to run constantly.
 *
 * When idle_notify is set, the hook will run every cycle of the
 * main-loop.  Of course, nothing will happen unless the user
 * has trapped the event, and it's not blocked.
 *
 * Opposite of eva_hook_clear_idle_notify().
 */
void
eva_hook_mark_idle_notify (EvaHook        *hook)
{
  g_return_if_fail (!EVA_HOOK_TEST_FLAG (hook, JUST_NEVER_BLOCKS));
  if (!EVA_HOOK_TEST_FLAG (hook, IS_AVAILABLE))
    return;
  if (EVA_HOOK_TEST_FLAG (hook, IDLE_NOTIFY))
    return;

  DEBUG ((hook, "eva_hook_mark_idle_notify"));

  EVA_HOOK_SET_FLAG (hook, IDLE_NOTIFY);
  if (EVA_HOOK_TEST_FLAG (hook, HAS_POLL))
    eva_hook_add_to_nonblocking_list (hook);
}

/**
 * eva_hook_clear_idle_notify:
 * @hook: the hook to stop running constantly.
 *
 * Stop running the hook at every cycle of the main-loop.
 *
 * Opposite of eva_hook_set_idle_notify().
 */
void
eva_hook_clear_idle_notify (EvaHook        *hook)
{
  g_return_if_fail (!EVA_HOOK_TEST_FLAG (hook, JUST_NEVER_BLOCKS));
  if (!EVA_HOOK_TEST_FLAG (hook, IDLE_NOTIFY))
    return;
  DEBUG ((hook, "eva_hook_clear_idle_notify"));
  _eva_hook_clear_idle_notify (hook);
}

/**
 * eva_hook_mark_never_blocks:
 * @hook: the hook which will never block.
 *
 * Indicate that you will never block, ever.
 */

void
eva_hook_mark_never_blocks  (EvaHook        *hook)
{
  eva_hook_mark_idle_notify (hook);
  EVA_HOOK_SET_FLAG (hook, JUST_NEVER_BLOCKS);
}

/**
 * eva_hook_mark_can_defer_shutdown:
 * @hook: the hook which can defer shutdown.
 *
 * Indicate that the hook may remain in the SHUTTING_DOWN state after the
 * class's shutdown method is called, deferring shutdown notification until
 * a later time.
 *
 * If you set this flag, your class's shutdown method must return FALSE
 * if shutdown notification is deferred, or TRUE if shutdown is complete
 * and the user should be notified immediately; failure is indicated only
 * by setting the GError parameter to the shutdown method.  (Therefore,
 * setting this flag has no effect unless you also set SHUTDOWN_HAS_ERROR.)
 */
void
eva_hook_mark_can_defer_shutdown (EvaHook *hook)
{
  EVA_HOOK_SET_FLAG (hook, CAN_DEFER_SHUTDOWN);
}

/**
 * eva_hook_destruct:
 * @hook: the hook to destroy.
 *
 * This should be called only by the class which contains the hook,
 * from the instance's finalize method.
 */
void
eva_hook_destruct        (EvaHook        *hook)
{
  if (hook->flags & EVA_HOOK_HAS_POLL)
    eva_hook_clear_poll (hook);
  if (hook->destroy)
    hook->destroy (hook->data);
}

/* per-class initialization */
typedef struct {
  GType type;
  const char *name;
} PerHookTypeInfo;
typedef struct {
  guint num_info;
  PerHookTypeInfo type_infos[1];	/* flexible array */
} PerOffsetInfo;

static GPtrArray *per_offset = NULL;

/**
 * eva_hook_class_init:
 * @object_class: the class of the object which contains this hook.
 * @name: an identifying name for this type of hook.
 * @hook_offset: the offset of the hook in the structure.
 *
 * This is used to register a hook.  This is mostly used
 * to make debugging printouts easier to read.
 */
void
eva_hook_class_init     (GObjectClass   *object_class,
			 const char     *name,
			 guint           hook_offset)
{
  guint index;
  PerOffsetInfo *oi;
  g_assert (hook_offset % 4 == 0);
  g_assert (hook_offset >= sizeof (GObject));
  index = (hook_offset - sizeof (GObject)) / 4;
  if (per_offset->len <= index)
    g_ptr_array_set_size (per_offset, index + 1);
  oi = per_offset->pdata[index];
  if (!oi)
    {
      oi = g_new (PerOffsetInfo, 1);
      oi->num_info = 0;
    }
  else
    {
      guint size = sizeof (PerOffsetInfo)
	         + sizeof (PerHookTypeInfo) * (oi->num_info);
      oi = g_realloc (oi, size);
    }
  per_offset->pdata[index] = oi;
  oi->type_infos[oi->num_info].type = G_OBJECT_CLASS_TYPE (object_class);
  oi->type_infos[oi->num_info].name = name;
  ++(oi->num_info);
}

/**
 * eva_hook_get_last_poll_state:
 * @hook: test whether the last invocation of the set-poll function
 * was called with TRUE or FALSE.
 *
 * Get whether this hook is supposed to be polling or not.
 *
 * returns: whether the hook is being polled.
 */
gboolean
eva_hook_get_last_poll_state(EvaHook       *hook)
{
  return EVA_HOOK_TEST_FLAG (hook, HAS_POLL);
}

#ifdef EVA_DEBUG
static const char *
get_hook_name_and_type (EvaHook        *hook,
			GType          *type_out)
{
  guint index = hook->inset;
  PerOffsetInfo *oi;
  GObject *object;
  GType object_type;
  guint i;
  g_assert (index % 4 == 0 && index >= sizeof (GObject));
  index -= sizeof (GObject);
  index /= 4;
  if (per_offset->len <= index)
    goto fail;
  oi = per_offset->pdata[index];
  object = EVA_HOOK_GET_OBJECT (hook);
  object_type = G_OBJECT_TYPE (object);
  for (i = 0; i < oi->num_info; i++)
    if (g_type_is_a (object_type, oi->type_infos[i].type))
      {
	*type_out = oi->type_infos[i].type;
	return oi->type_infos[i].name;
      }
fail:
  *type_out = 0;
  return NULL;
}

#include <stdio.h>
static void
eva_hook_debug (EvaHook    *hook,
		const char *format,
		...)
{
  GType hook_type;
  const char *name = get_hook_name_and_type (hook, &hook_type);
  GObject *instance = EVA_HOOK_GET_OBJECT (hook);
  GType instance_type = G_OBJECT_TYPE (instance);
  va_list args;
  fprintf (stderr, "debug: hook: %s:%s [%s,%p]: ",
	   g_type_name (hook_type), name,
           g_type_name (instance_type), instance);
  va_start (args, format);
  vfprintf (stderr, format, args);
  va_end (args);
  fprintf (stderr, ".\n");
}
#endif


/**
 * _eva_hook_init:
 *
 * Initialize the hook system.
 */
void  _eva_hook_init ()
{
  per_offset = g_ptr_array_new ();
  eva_hook_main_loop_quark = g_quark_from_static_string ("EvaHook--main-loop-nonblocking");
}
