#ifndef SRC_EVA_EVAHELLO_H_
#define SRC_EVA_EVAHELLO_H_
#include <glib-object.h>
/*
 * Potentially, include other headers on which this header depends.
 */

/*
 * Type macros.
 */
#define EVA_TYPE_HELLO                  (eva_hello_get_type ())
#define EVA_HELLO(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_HELLO, EvaHello))
#define EVA_IS_HELLO(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_HELLO))
#define EVA_HELLO_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_HELLO, EvaHelloClass))
#define EVA_IS_HELLO_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_HELLO))
#define EVA_HELLO_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_HELLO, EvaHelloClass))
/* extra */
#define EVA_HELLO_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), EVA_TYPE_HELLO, EvaHelloPrivate))

typedef struct _EvaHello        EvaHello;
typedef struct _EvaHelloClass   EvaHelloClass;
typedef struct _EvaHelloPrivate EvaHelloPrivate;

struct _EvaHelloPrivate
{
  int hsize;
};

struct _EvaHello
{
  GObject parent_instance;

  /*< private >*/
  EvaHelloPrivate *priv;
};

struct _EvehelloHelloClass
{
  GObjectClass parent_class;

  /* class members */
};

static void
eva_hello_class_init (EvaHelloClass *klass)
{
  g_type_class_add_private (klass, sizeof (EvaHelloPrivate));
}

/* used by EVA_TYPE_HELLO */
GType eva_hello_get_type (void);

static void
eva_hello_init (EvaHello *self)
{
  EvaHelloPrivate *priv;
  self->priv = priv = EVA_HELLO_GET_PRIVATE (self);
  priv->hsize = 42;
}

/*
 * Method definitions.
 */
void eva_hello_do_action(EvaHello *self /*, parameters */);

#endif  // SRC_EVA_EVAHELLO_H_

