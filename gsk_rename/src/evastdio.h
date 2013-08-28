#ifndef __EVA_STDIO_H_
#define __EVA_STDIO_H_

/* stdio helper functions */
#include <stdio.h>
#include <glib.h>

G_BEGIN_DECLS

gchar * eva_stdio_readline (FILE *fp);

G_END_DECLS

#endif
