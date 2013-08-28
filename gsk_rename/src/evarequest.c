#define G_IMPLEMENT_INLINES 1
#define __EVA_REQUEST_C__
#include "evarequest.h"
#include "evadebug.h"
#include "debug.h"

#if defined (EVA_DEBUG)
static void eva_request_debug (EvaRequest *request,
			       const char *format,
			       ...) G_GNUC_PRINTF(2,3);
#define DEBUG(args) \
  G_STMT_START{ \
    if (EVA_IS_DEBUGGING (REQUEST)) \
      eva_request_debug args; \
  }G_STMT_END
#else
#define DEBUG(args)
#endif

static guint done_signal = 0;
static guint cancelled_signal = 0;

/* Default signal handler. */
static void
eva_request_default_cancelled (EvaRequest *request)
{
  eva_request_mark_is_cancelled (request);
}

/**
 * eva_request_start:
 * @request: the #EvaRequest to start.
 *
 * Start a #EvaRequest.
 */
void
eva_request_start (gpointer request)
{
  EvaRequestClass *request_class;

  g_return_if_fail (request);
  g_return_if_fail (EVA_IS_REQUEST (request));
  request_class = EVA_REQUEST_GET_CLASS (request);
  g_return_if_fail (request_class);
  g_return_if_fail (request_class->start);

  g_return_if_fail (!eva_request_get_is_running (request));
  g_return_if_fail (!eva_request_get_is_cancelled (request));
  g_return_if_fail (!eva_request_get_is_done (request));

  DEBUG ((request, "starting"));

  (*request_class->start) (EVA_REQUEST (request));
}

/**
 * eva_request_cancel:
 * @request: the #EvaRequest to cancel.
 *
 * Cancel a running #EvaRequest.
 */
void
eva_request_cancel (gpointer request)
{
  g_return_if_fail (eva_request_get_is_cancellable (request));
  g_return_if_fail (!eva_request_get_is_cancelled (request));
  DEBUG ((request, "cancelling"));
  g_signal_emit (request, cancelled_signal, 0);
}

/**
 * eva_request_set_error:
 * @request: the #EvaRequest to set the error for.
 * @error: the #GError.
 *
 * Set the error member of a #EvaRequest.
 *
 * Protected; this function should only be used by subclasses of
 * #EvaRequest.
 */
void
eva_request_set_error (gpointer ptr, GError *error)
{
  EvaRequest *request = EVA_REQUEST (ptr);

  g_return_if_fail (request);
  g_return_if_fail (EVA_IS_REQUEST (request));
  g_return_if_fail (error);

  DEBUG ((request, "setting error: %s", error ? error->message : "(NULL)"));
  if (request->error)
    g_error_free (request->error);
  request->error = error;
}

/**
 * eva_request_done:
 * @request: the #EvaRequest which has completed.
 *
 * Mark the request as done; emit the "done" signal to notify clients.
 *
 * Protected; this function should only be used by subclasses of
 * #EvaRequest.
 */
void
eva_request_done (gpointer request)
{
  g_return_if_fail (EVA_IS_REQUEST (request));
  g_return_if_fail (!eva_request_get_is_cancelled (request));
  g_return_if_fail (!eva_request_get_is_done (request));
  eva_request_clear_is_running (request);
  eva_request_mark_is_done (request);
  DEBUG ((request, "done"));
  g_signal_emit (request, done_signal, 0);
}

static void
eva_request_class_init (EvaRequestClass *request_class)
{
  GType type = G_OBJECT_CLASS_TYPE (request_class);

  /* Default signal handler. */
  request_class->cancelled = eva_request_default_cancelled;

  done_signal =
    g_signal_new ("done",
		  type,
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
		  G_STRUCT_OFFSET (EvaRequestClass, done),
		  NULL,		/* accumulator */
		  NULL,		/* accu_data */
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE,
		  0);
  cancelled_signal =
    g_signal_new ("cancelled",
		  type,
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
		  G_STRUCT_OFFSET (EvaRequestClass, cancelled),
		  NULL,		/* accumulator */
		  NULL,		/* accu_data */
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE,
		  0);
}

GType
eva_request_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0))
    {
      static const GTypeInfo type_info =
	{
	  sizeof (EvaRequestClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) eva_request_class_init,
	  NULL,		/* class_finalize */
	  NULL,		/* class_data */
	  sizeof (EvaRequest),
	  0,		/* n_preallocs */
	  (GInstanceInitFunc) NULL,
	  NULL		/* value_table */
	};
      type = g_type_register_static (G_TYPE_OBJECT,
				     "EvaRequest",
				     &type_info,
				     G_TYPE_FLAG_ABSTRACT);
    }
  return type;
}

#if defined (EVA_DEBUG)
#include <stdio.h>
static void
eva_request_debug (EvaRequest *request,
		   const char *format,
		   ...)
{
  va_list args;

  fprintf (stderr, "debug: request [%s/%p]: ",
	   g_type_name (G_OBJECT_TYPE (request)),
	   request);
  va_start (args, format);
  vfprintf (stderr, format, args);
  va_end (args);
  fprintf (stderr, ".\n");
}
#endif
