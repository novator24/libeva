#ifndef __EVA_IO_H_
#define __EVA_IO_H_

#include <glib-object.h>
#include "evahook.h"
#include "evaerror.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _GskIO GskIO;
typedef struct _GskIOClass GskIOClass;

typedef gboolean (*GskIOHookFunc) (GskIO        *io,
				   gpointer      data);
/* --- type macros --- */
GType eva_io_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_IO		 (eva_io_get_type ())
#define EVA_IO(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_IO, GskIO))
#define EVA_IO_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_IO, GskIOClass))
#define EVA_IO_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_IO, GskIOClass))
#define EVA_IS_IO(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_IO))
#define EVA_IS_IO_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_IO))

typedef enum
{
  EVA_IO_ERROR_NONE,
  EVA_IO_ERROR_INIT,
  EVA_IO_ERROR_CONNECT,
  EVA_IO_ERROR_OPEN,
  EVA_IO_ERROR_READ,
  EVA_IO_ERROR_WRITE,
  EVA_IO_ERROR_POLL_READ,
  EVA_IO_ERROR_POLL_WRITE,
  EVA_IO_ERROR_SHUTDOWN_READ,
  EVA_IO_ERROR_SHUTDOWN_WRITE,
  EVA_IO_ERROR_CLOSE,
  EVA_IO_ERROR_SYNC,
  EVA_IO_ERROR_POLL
} GskIOErrorCause;
const char * eva_io_error_cause_to_string (GskIOErrorCause cause);

/* --- structures --- */
struct _GskIOClass 
{
  GObjectClass object_class;
  /* --- signals (do not override, usually) --- */
  /* Emitted after the connection is made. */
  void       (*on_connect)      (GskIO      *io);
  void       (*on_error)        (GskIO      *io);

  /* --- virtuals --- */
  gboolean   (*open)            (GskIO      *io,
				 GError    **error);
  void       (*set_poll_read)   (GskIO      *io,
				 gboolean    do_poll);
  void       (*set_poll_write)  (GskIO      *io,
				 gboolean    do_poll);
  gboolean   (*shutdown_read)   (GskIO      *io,
				 GError    **error);
  gboolean   (*shutdown_write)  (GskIO      *io,
				 GError    **error);
  void       (*close)           (GskIO      *io);
};
struct _GskIO 
{
  GObject      object;

  guint        is_connecting : 1;
  guint        is_open : 1;
  guint        open_failed : 1;
  guint        shutdown_on_error : 1;

  /*< private >*/
  guint        in_idle_ready_thread : 1;

  /*< public read-only >*/
  guint        error_cause : 4;

  /*< public read-write >*/
  guint        print_errors : 1;

  /*< public read-only >*/
  GError      *error;

  /*< private >*/
  /* hooks */
  GskHook           read_hook;
  GskHook           write_hook;
};
/* --- prototypes --- */

/* public */
#define EVA_IO_READ_HOOK(io)		((GskHook*) &EVA_IO (io)->read_hook)
#define EVA_IO_WRITE_HOOK(io)		((GskHook*) &EVA_IO (io)->write_hook)
#define eva_io_block_read(io)		eva_hook_block (EVA_IO_READ_HOOK (io))
#define eva_io_block_write(io)		eva_hook_block (EVA_IO_WRITE_HOOK (io))
#define eva_io_unblock_read(io)		eva_hook_unblock (EVA_IO_READ_HOOK (io))
#define eva_io_unblock_write(io)	eva_hook_unblock (EVA_IO_WRITE_HOOK (io))
#define eva_io_has_read_hook(io)        eva_hook_is_trapped (EVA_IO_READ_HOOK (io))
#define eva_io_has_write_hook(io)       eva_hook_is_trapped (EVA_IO_WRITE_HOOK (io))
#define eva_io_trap_readable(io, func, shutdown_func, data, destroy)		\
	eva_hook_trap (EVA_IO_READ_HOOK (io),					\
		       (GskHookFunc) func, (GskHookFunc) shutdown_func, data, destroy)
#define eva_io_trap_writable(io, func, shutdown_func, data, destroy)		\
	eva_hook_trap (EVA_IO_WRITE_HOOK (io),					\
		       (GskHookFunc) func, (GskHookFunc) shutdown_func, data, destroy)
#define eva_io_untrap_readable(io) eva_hook_untrap (EVA_IO_READ_HOOK (io))
#define eva_io_untrap_writable(io) eva_hook_untrap (EVA_IO_WRITE_HOOK (io))
void eva_io_shutdown (GskIO   *io,
		      GError **error);

/* shutdown the io in various ways */             
#define eva_io_read_shutdown(io, error)   eva_hook_shutdown (EVA_IO_READ_HOOK (io), error)
#define eva_io_write_shutdown(io, error)  eva_hook_shutdown (EVA_IO_WRITE_HOOK (io), error)
void     eva_io_close             (GskIO          *io);


/* protected: do notifications of the usual events */
#define eva_io_notify_ready_to_read(io)		eva_hook_notify (EVA_IO_READ_HOOK (io))
#define eva_io_notify_ready_to_write(io)	eva_hook_notify (EVA_IO_WRITE_HOOK (io))
#define eva_io_notify_read_shutdown(io)		eva_hook_notify_shutdown (EVA_IO_READ_HOOK (io))
#define eva_io_notify_write_shutdown(io)	eva_hook_notify_shutdown (EVA_IO_WRITE_HOOK (io))
void     eva_io_notify_shutdown (GskIO *io);
void     eva_io_notify_connected (GskIO *io);

/* flags */
#define eva_io_get_is_connecting(io)             _EVA_IO_TEST_FIELD (io, is_connecting)
#define eva_io_get_is_readable(io)               _EVA_IO_TEST_READ_FLAG (io, IS_AVAILABLE)
#define eva_io_get_is_writable(io)               _EVA_IO_TEST_WRITE_FLAG (io, IS_AVAILABLE)
#define eva_io_get_is_read_shutting_down(io)     EVA_HOOK_TEST_SHUTTING_DOWN (EVA_IO_READ_HOOK (io))
#define eva_io_get_is_write_shutting_down(io)    EVA_HOOK_TEST_SHUTTING_DOWN (EVA_IO_WRITE_HOOK (io))
#define eva_io_get_never_partial_reads(io)       _EVA_IO_TEST_FIELD (io, never_partial_reads)
#define eva_io_get_never_partial_writes(io)      _EVA_IO_TEST_FIELD (io, never_partial_writes)
#define eva_io_get_never_blocks_write(io)        EVA_HOOK_TEST_NEVER_BLOCKS (EVA_IO_WRITE_HOOK (io))
#define eva_io_get_never_blocks_read(io)         EVA_HOOK_TEST_NEVER_BLOCKS (EVA_IO_READ_HOOK (io))
#define eva_io_get_idle_notify_write(io)         EVA_HOOK_TEST_IDLE_NOTIFY (EVA_IO_WRITE_HOOK (io))
#define eva_io_get_idle_notify_read(io)          EVA_HOOK_TEST_IDLE_NOTIFY (EVA_IO_READ_HOOK (io))
#define eva_io_get_is_open(io)                   _EVA_IO_TEST_FIELD (io, is_open)
#define eva_io_get_shutdown_on_error(io)         _EVA_IO_TEST_FIELD (io, shutdown_on_error)

/* protected */
#define eva_io_set_idle_notify_read(io,v)        eva_hook_set_idle_notify (EVA_IO_READ_HOOK (io),v)
#define eva_io_set_idle_notify_write(io,v)       eva_hook_set_idle_notify (EVA_IO_WRITE_HOOK (io),v)
#define eva_io_mark_is_connecting(io)            _EVA_IO_MARK_FIELD (io, is_connecting)
#define eva_io_mark_is_readable(io)              _EVA_IO_SET_READ_FLAG (io, IS_AVAILABLE)
#define eva_io_mark_is_writable(io)              _EVA_IO_SET_WRITE_FLAG (io, IS_AVAILABLE)
#define eva_io_mark_never_partial_reads(io)      _EVA_IO_MARK_FIELD (io, never_partial_reads)
#define eva_io_mark_never_partial_writes(io)     _EVA_IO_MARK_FIELD (io, never_partial_writes)
#define eva_io_mark_shutdown_on_error(io)        _EVA_IO_MARK_FIELD (io, shutdown_on_error)
#define eva_io_mark_never_blocks_write(io)       eva_hook_mark_never_blocks (EVA_IO_WRITE_HOOK (io))
#define eva_io_mark_never_blocks_read(io)        eva_hook_mark_never_blocks (EVA_IO_READ_HOOK (io))
#define eva_io_mark_idle_notify_write(io)        eva_hook_mark_idle_notify (EVA_IO_WRITE_HOOK (io))
#define eva_io_mark_idle_notify_read(io)         eva_hook_mark_idle_notify (EVA_IO_READ_HOOK (io))
#define eva_io_mark_is_open(io)                  _EVA_IO_MARK_FIELD (io, is_open)
#define eva_io_clear_is_readable(io)             _EVA_IO_CLEAR_READ_FLAG (io, IS_AVAILABLE)
#define eva_io_clear_is_writable(io)             _EVA_IO_CLEAR_WRITE_FLAG (io, IS_AVAILABLE)
#define eva_io_clear_never_partial_reads(io)     _EVA_IO_CLEAR_FIELD (io, never_partial_reads)
#define eva_io_clear_never_partial_writes(io)    _EVA_IO_CLEAR_FIELD (io, never_partial_writes)
#define eva_io_clear_idle_notify_write(io)       eva_hook_clear_idle_notify (EVA_IO_WRITE_HOOK (io))
#define eva_io_clear_idle_notify_read(io)        eva_hook_clear_idle_notify (EVA_IO_READ_HOOK (io))
#define eva_io_clear_is_open(io)                 _EVA_IO_CLEAR_FIELD (io, is_open)
#define eva_io_clear_shutdown_on_error(io)       _EVA_IO_CLEAR_FIELD (io, shutdown_on_error)
#define eva_io_is_polling_for_read(io)           eva_hook_get_last_poll_state (EVA_IO_READ_HOOK (io))
#define eva_io_is_polling_for_write(io)          eva_hook_get_last_poll_state (EVA_IO_WRITE_HOOK (io))




/* --- error handling --- */

/* these functions are for use by derived classes only. */
void        eva_io_set_error (GskIO             *io,
                              GskIOErrorCause    cause,
                              GskErrorCode       error_code,
                              const char        *format,
                              ...) G_GNUC_PRINTF(4,5);
void        eva_io_set_gerror (GskIO             *io,
                               GskIOErrorCause    cause,
                               GError            *error);

void        eva_io_set_default_print_errors (gboolean print_errors);

/*< private >*/
void _eva_io_stop_idle_ready (GskIO *io);
void _eva_io_make_idle_ready (GskIO *io);
void _eva_io_nonblocking_init ();

/* implementation bits */
#define _EVA_IO_TEST_FIELD(io, field)	        (EVA_IO (io)->field != 0)
#define _EVA_IO_TEST_READ_FLAG(io, flag)        EVA_HOOK_TEST_FLAG(EVA_IO_READ_HOOK(io), flag)
#define _EVA_IO_TEST_WRITE_FLAG(io, flag)       EVA_HOOK_TEST_FLAG(EVA_IO_WRITE_HOOK(io), flag)
#define _EVA_IO_MARK_FIELD(io, field)	        G_STMT_START{ EVA_IO (io)->field = 1; }G_STMT_END
#define _EVA_IO_MARK_READ_FLAG(io, flag)        EVA_HOOK_MARK_FLAG(EVA_IO_READ_HOOK(io), flag)
#define _EVA_IO_MARK_WRITE_FLAG(io, flag)       EVA_HOOK_MARK_FLAG(EVA_IO_WRITE_HOOK(io), flag)
#define _EVA_IO_CLEAR_FIELD(io, field)	        G_STMT_START{ EVA_IO (io)->field = 0; }G_STMT_END
#define _EVA_IO_CLEAR_READ_FLAG(io, flag)       EVA_HOOK_CLEAR_FLAG(EVA_IO_READ_HOOK(io), flag)
#define _EVA_IO_CLEAR_WRITE_FLAG(io, flag)      EVA_HOOK_CLEAR_FLAG(EVA_IO_WRITE_HOOK(io), flag)

#ifndef EVA_DISABLE_DEPRECATED
#define _EVA_IO_SET_READ_FLAG _EVA_IO_MARK_READ_FLAG
#define _EVA_IO_SET_WRITE_FLAG _EVA_IO_MARK_WRITE_FLAG
#endif

G_END_DECLS

#endif
