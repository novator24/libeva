#ifndef __EVA_TYPES_H_
#define __EVA_TYPES_H_

#include <glib-object.h>

G_BEGIN_DECLS

GType eva_fd_get_type () G_GNUC_CONST;
GType eva_param_fd_get_type () G_GNUC_CONST;
#define EVA_TYPE_FD		(eva_fd_get_type())
#define EVA_TYPE_PARAM_FD	(eva_param_fd_get_type())

GParamSpec *eva_param_spec_fd (const char *name,
			       const char *nick,
			       const char *blurb,
			       GParamFlags flags);

G_END_DECLS

#endif
