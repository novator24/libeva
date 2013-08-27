#ifndef __EVA_PERSISTENT_CONNECTION_H_
#define __EVA_PERSISTENT_CONNECTION_H_

#include "evastream.h"
#include "evamainloop.h"
#include "evasocketaddress.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _GskPersistentConnection GskPersistentConnection;
typedef struct _GskPersistentConnectionClass GskPersistentConnectionClass;
/* --- type macros --- */
GType eva_persistent_connection_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_PERSISTENT_CONNECTION			(eva_persistent_connection_get_type ())
#define EVA_PERSISTENT_CONNECTION(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_PERSISTENT_CONNECTION, GskPersistentConnection))
#define EVA_PERSISTENT_CONNECTION_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_PERSISTENT_CONNECTION, GskPersistentConnectionClass))
#define EVA_PERSISTENT_CONNECTION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_PERSISTENT_CONNECTION, GskPersistentConnectionClass))
#define EVA_IS_PERSISTENT_CONNECTION(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_PERSISTENT_CONNECTION))
#define EVA_IS_PERSISTENT_CONNECTION_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_PERSISTENT_CONNECTION))

/* --- structures --- */
/* NOTE: the "DOING_NAME_RESOLUTION" state is now obsolete,
   but it is retained for backward compatibility.
   Use the eva_persistent_connection_is_doing_name_resolution()
   macro, and know that when doing name-resolution,
   state == CONNECTING. */
typedef enum
{
  EVA_PERSISTENT_CONNECTION_INIT,
  EVA_PERSISTENT_CONNECTION_DOING_NAME_RESOLUTION,
  EVA_PERSISTENT_CONNECTION_CONNECTING,
  EVA_PERSISTENT_CONNECTION_CONNECTED,
  EVA_PERSISTENT_CONNECTION_WAITING
} GskPersistentConnectionState;

struct _GskPersistentConnectionClass 
{
  GskStreamClass base_class;
  void (*handle_connected)    (GskPersistentConnection *);
  void (*handle_disconnected) (GskPersistentConnection *);
};

struct _GskPersistentConnection 
{
  GskStream      base_instance;

  GskPersistentConnectionState state;
  guint             retry_timeout_ms;

  /* debugging */
  guint             warn_on_transport_errors : 1;
  guint             debug_connection : 1;

  /* Alternate methods for specifying the address. */

  /* by socket address */
  GskSocketAddress *address;

  /*< private >*/
  GskStream        *transport;
  GskSource        *retry_timeout_source;
  gulong transport_on_connect_signal_handler;
  gulong transport_on_error_signal_handler;
};

/* note: you will have to #include streamfd.h for this to work. */
#define eva_persistent_connection_is_doing_name_resolution(pc) \
  ((pc)->state == EVA_PERSISTENT_CONNECTION_CONNECTING         \
   && EVA_STREAM_FD ((pc)->transport)->is_resolving_name)

/* --- prototypes --- */
GskStream *eva_persistent_connection_new (GskSocketAddress *address,
                                          guint             retry_timeout_ms);
GskStream *eva_persistent_connection_new_lookup
                                     (const char *host,
                                      guint       port,
                                      guint       retry_timeout_ms);

void eva_persistent_connection_restart (GskPersistentConnection *connection,
                                        guint                    retry_wait_ms);


G_END_DECLS

#endif
