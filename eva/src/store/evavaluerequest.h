#ifndef __EVA_VALUE_REQUEST_H_
#define __EVA_VALUE_REQUEST_H_

#include "../evarequest.h"

G_BEGIN_DECLS

typedef EvaRequestClass         EvaValueRequestClass;
typedef struct _EvaValueRequest EvaValueRequest;

GType eva_value_request_get_type (void) G_GNUC_CONST;

#define EVA_TYPE_VALUE_REQUEST (eva_value_request_get_type ())
#define EVA_VALUE_REQUEST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_VALUE_REQUEST, EvaValueRequest))
#define EVA_IS_VALUE_REQUEST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_VALUE_REQUEST))

struct _EvaValueRequest
{
  EvaRequest request;

  GValue value;
};

G_INLINE_FUNC
G_CONST_RETURN GValue * eva_value_request_get_value (gpointer request);

#if defined (G_CAN_INLINE) || defined (__EVA_VALUE_REQUEST_C__)

G_INLINE_FUNC G_CONST_RETURN GValue *
eva_value_request_get_value (gpointer request)
{
  EvaValueRequest *value_request = EVA_VALUE_REQUEST (request);

  g_return_val_if_fail (!eva_request_get_is_running (request), NULL);
  g_return_val_if_fail (!eva_request_get_is_cancelled (request), NULL);
  g_return_val_if_fail (eva_request_get_is_done (request), NULL);
  g_return_val_if_fail (!eva_request_had_error (request), NULL);
  return &value_request->value;
}

#endif /* defined (G_CAN_INLINE) || defined (__EVA_VALUE_REQUEST_C__) */

G_END_DECLS

#endif
