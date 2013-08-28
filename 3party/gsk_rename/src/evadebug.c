#include "debug.h"

#ifdef EVA_DEBUG
const gboolean eva_debugging_on = 1;
#else
const gboolean eva_debugging_on = 0;
#endif

/**
 * eva_debug_set_flags:
 * @flags: debug bits to start logging.
 *
 * Set which types of debug logs to emit.
 */
void
eva_debug_set_flags (EvaDebugFlags flags)
{
#ifdef EVA_DEBUG
  eva_debug_flags = flags;
#endif
}

/**
 * eva_debug_add_flags:
 * @flags: debug bits to start logging.
 *
 * Add new types of debug logs to emit.
 */
void
eva_debug_add_flags (EvaDebugFlags flags)
{
#ifdef EVA_DEBUG
  eva_debug_flags |= flags;
#endif
}

EvaDebugFlags eva_debug_flags = 0;
