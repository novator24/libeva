#ifndef __EVA_LOG_RING_BUFFER_H_
#define __EVA_LOG_RING_BUFFER_H_

typedef struct _GskLogRingBuffer GskLogRingBuffer;

#include <glib.h>

G_BEGIN_DECLS

GskLogRingBuffer *eva_log_ring_buffer_new (gsize             size);
void              eva_log_ring_buffer_add (GskLogRingBuffer *buffer,
                                           const char       *line);

/* get the contents of the ring-buffer as a NUL-terminated string */
char             *eva_log_ring_buffer_get (const GskLogRingBuffer *buffer);

void              eva_log_ring_buffer_free(GskLogRingBuffer *buffer);


/* This is a little helper function to rewrite strings
   that look like:
       unixtime whatever;
   this converts the localtime into humanreadable format.
   This uses the localtime() function,
   which in turn uses $TZ then /etc/timezone
   for its timezone info. */
char       * eva_substitute_localtime_in_string (const char *str,
                                                 const char *strftime_format);
G_END_DECLS

#endif
