#ifndef __EVA_HOOK_H_
#define __EVA_HOOK_H_

#include <glib-object.h>

/* GskHook: a blockable, optionally shutdown-able, hookable thing
 *          inside an object.
 *
 *  well, there's a bunch of other constraints;
 *  in many ways this is just a bunch of private details of evaio,
 *  but i thought it might be generally useful by encapsulating
 *  it in this way.
 *
 *  
 *  the encapsulation here is pretty minimal:  GskHook must be a
 *  member of a GObject-derived class.  it must have a member to
 *  trap/untrap reading/writing (unless it never blocks)
 *  and it may have an optional shutdown method.
 *
 *  specifically, your Class should probably contain functions like
 *  
 *      void (*set_poll) (Object    *object,
 *                        gboolean   do_polling);
 *      void (*shutdown) (Object    *object);
 * or, if SHUTDOWN_HAS_ERROR,
 *      gboolean (*shutdown) (Object    *object,
 *                            GError   **error);
 *
 */

G_BEGIN_DECLS

typedef struct _GskHook GskHook;

typedef gboolean (*GskHookFunc) (GObject     *object,
				 gpointer     data);

/* just for reference: these are the prototypes of the two members
 * described above that may/should appear in your Class structure. */
typedef void     (*GskHookSetPollFunc) (GObject      *object,
					gboolean      do_polling);
typedef void     (*GskHookShutdownFunc)(GObject      *object);
typedef gboolean (*GskHookShutdownErrorFunc)(GObject      *object,
					     GError      **error);


/* note: fits in 16 bits */
typedef enum
{
  EVA_HOOK_IS_AVAILABLE            	= (1 << 0),
  EVA_HOOK_NEVER_AUTO_SHUTS_DOWN   	= (1 << 1),
  EVA_HOOK_CAN_HAVE_SHUTDOWN_ERROR 	= (1 << 2),
  EVA_HOOK_private_IDLE_NOTIFY     	= (1 << 3), /*< private >*/
  EVA_HOOK_private_JUST_NEVER_BLOCKS    = (1 << 4), /*< private >*/
  EVA_HOOK_private_NEVER_BLOCKS		= (EVA_HOOK_private_IDLE_NOTIFY | EVA_HOOK_private_JUST_NEVER_BLOCKS), /*< private >*/
  EVA_HOOK_private_CAN_DEFER_SHUTDOWN   = (1 << 5), /*< private >*/
  EVA_HOOK_private_SHUTTING_DOWN        = (1 << 6), /*< private >*/
  _EVA_HOOK_FLAGS_RESERVED	   	= (0xff << 8), /*< private >*/
} GskHookFlags;

#define EVA_HOOK_TEST_FLAG(hook, flag_shortname)	\
	(((hook)->flags & EVA_HOOK_ ## flag_shortname) == EVA_HOOK_ ## flag_shortname)
#define EVA_HOOK_MARK_FLAG(hook, flag_shortname)		\
	((hook)->flags |= (EVA_HOOK_ ## flag_shortname))
#define EVA_HOOK_CLEAR_FLAG(hook, flag_shortname)	\
	((hook)->flags &= ~(EVA_HOOK_ ## flag_shortname))
#define EVA_HOOK_TEST_IDLE_NOTIFY(hook)  EVA_HOOK_TEST_FLAG(hook, private_IDLE_NOTIFY)
#define EVA_HOOK_TEST_NEVER_BLOCKS(hook) EVA_HOOK_TEST_FLAG(hook, private_NEVER_BLOCKS)
#define EVA_HOOK_TEST_IS_AVAILABLE(hook) EVA_HOOK_TEST_FLAG(hook, IS_AVAILABLE)
#define EVA_HOOK_TEST_SHUTTING_DOWN(hook) EVA_HOOK_TEST_FLAG(hook, private_SHUTTING_DOWN)

/*< protected >*/
#define EVA_HOOK_TEST_USER_FLAG(hook, bit)		\
  (((hook)->user_flags & (bit)) == (bit))
#define EVA_HOOK_MARK_USER_FLAG(hook, bit)		\
  ((hook)->user_flags |= (bit))
#define EVA_HOOK_CLEAR_USER_FLAG(hook, bit)		\
  ((hook)->user_flags &= ~(bit))

#define EVA_HOOK_GET_OBJECT(hook)	(G_OBJECT ((char *) (hook) - (hook)->inset))

struct _GskHook
{
  /*< private >*/
  guint16 flags;
  guint16 user_flags;		/* for use by containing class */
  guint16 block_count;
  guint16 inset;
  guint16 class_set_poll_offset;
  guint16 class_shutdown_offset;

  GskHookFunc func;
  GskHookFunc shutdown_func;
  gpointer data;
  GDestroyNotify destroy;
};


/*< public >*/
void     eva_hook_trap            (GskHook        *hook,
                                   GskHookFunc     func,
				   GskHookFunc     shutdown,
                                   gpointer        data,
                                   GDestroyNotify  destroy);
void     eva_hook_untrap          (GskHook        *hook);
#define eva_hook_is_trapped(hook) (((hook)->func) != NULL)
void     eva_hook_block           (GskHook        *hook);
void     eva_hook_unblock         (GskHook        *hook);
gboolean eva_hook_shutdown        (GskHook        *hook,
				   GError        **error);

/*< protected: for use by implementations of objects which have hooks >*/
void     eva_hook_init            (GskHook        *hook,
                                   GskHookFlags    flags,
                                   guint           inset,
                                   guint           class_set_poll_offset,
                                   guint           class_shutdown_offset);
void     eva_hook_class_init      (GObjectClass   *object_class,
				   const char     *name,
				   guint           hook_offset);
void     eva_hook_notify          (GskHook        *hook);
void     eva_hook_notify_shutdown (GskHook        *hook);
void     eva_hook_destruct        (GskHook        *hook);

void     eva_hook_set_idle_notify   (GskHook        *hook,
				     gboolean        should_idle_notify);
void     eva_hook_mark_idle_notify  (GskHook        *hook);
void     eva_hook_clear_idle_notify (GskHook        *hook);
void     eva_hook_mark_never_blocks  (GskHook        *hook);
void     eva_hook_mark_can_defer_shutdown (GskHook        *hook);
gboolean eva_hook_get_last_poll_state(GskHook       *hook);

/* macros for more conveniently initializing hooks from *_init */
#define EVA_HOOK_INIT(object, struct, member, flags, set_poll, shutdown)   \
        eva_hook_init (&(object)->member,                                  \
                       flags, G_STRUCT_OFFSET (struct, member),            \
                       G_STRUCT_OFFSET (struct ## Class, set_poll),        \
                       G_STRUCT_OFFSET (struct ## Class, shutdown))
#define EVA_HOOK_INIT_NO_SHUTDOWN(object, struct, member, flags, set_poll) \
        eva_hook_init (&(object)->member,                                  \
                       flags, G_STRUCT_OFFSET (struct, member),            \
                       G_STRUCT_OFFSET (struct ## Class, set_poll), 0)
#define EVA_HOOK_CLASS_INIT(object_class, hook_name, Type, member)	   \
  	eva_hook_class_init (object_class, hook_name,			   \
			     G_STRUCT_OFFSET (Type, member))

/* private: initialize the hook system (called by eva_init()) >*/
void  _eva_hook_init ();

#ifndef EVA_DISABLE_DEPRECATED
#define EVA_HOOK_SET_FLAG EVA_HOOK_MARK_FLAG
#endif

G_END_DECLS

#endif
