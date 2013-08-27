#ifndef __EVA_DEBUG_H_
#define __EVA_DEBUG_H_

#include <glib.h>

G_BEGIN_DECLS

typedef enum
{
  EVA_DEBUG_IO     		= (1<<0),
  EVA_DEBUG_STREAM 		= (1<<1),
  EVA_DEBUG_STREAM_LISTENER 	= (1<<2),
  EVA_DEBUG_STREAM_DATA         = (1<<3),
  EVA_DEBUG_LIFETIME		= (1<<4),
  EVA_DEBUG_MAIN_LOOP		= (1<<5),
  EVA_DEBUG_DNS			= (1<<6),
  EVA_DEBUG_HOOK		= (1<<7),
  EVA_DEBUG_SSL     		= (1<<8),
  EVA_DEBUG_HTTP     		= (1<<9),
  EVA_DEBUG_FTP     		= (1<<10),
  EVA_DEBUG_REQUEST 		= (1<<11),
  EVA_DEBUG_FD                  = (1<<12),

  EVA_DEBUG_ALL			= 0xffffffff
} GskDebugFlags;
/* note: if you add to this table, you should modify evainit.c */

/* Depends on whether --enable-eva-debug was specified to configure.  */
extern const gboolean eva_debugging_on;

void eva_debug_set_flags (GskDebugFlags flags);
void eva_debug_add_flags (GskDebugFlags flags);

/* read-only */
extern GskDebugFlags eva_debug_flags;

G_END_DECLS

#endif
