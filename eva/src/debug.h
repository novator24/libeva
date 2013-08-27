
#include "config.h"
#include "evadebug.h"

/* Implement _EVA_DEBUG_PRINTF to output a g_message in
 * accordance with certain GskDebugFlags.
 */
#ifdef EVA_DEBUG
#define _EVA_DEBUG_PRINTF(flags,args) 				\
	G_STMT_START{						\
	  if ((eva_debug_flags & (flags)) != 0)			\
	    g_message args;					\
	}G_STMT_END
#else
#define _EVA_DEBUG_PRINTF(flags,args)
#endif

#ifdef EVA_DEBUG
#define EVA_IS_DEBUGGING(short_name)	((eva_debug_flags & EVA_DEBUG_ ## short_name) == EVA_DEBUG_ ## short_name)
#else
#define EVA_IS_DEBUGGING(short_name)	FALSE
#endif

