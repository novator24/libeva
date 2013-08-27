#ifndef __EVA_DEBUG_MEMORY_LOG_H_
#define __EVA_DEBUG_MEMORY_LOG_H_

#include <glib.h>
#include <stdio.h>

G_BEGIN_DECLS

typedef struct _GskDebugLogMap GskDebugLogMap;
typedef struct _GskDebugLog GskDebugLog;

struct _GskDebugLog
{
  FILE *fp;
  gboolean is_64bit;
  gboolean little_endian;

  guint64 timestamp;

  guint n_maps;
  GskDebugLogMap *maps;

  GHashTable *context_cache;
};

typedef struct _GskDebugLogContext GskDebugLogContext;
struct _GskDebugLogContext
{
  guint64 address;      /* the key */
  char *desc;           /* the value */
};

struct _GskDebugLogMap
{
  guint64 start, length;
  char *path;
};


typedef enum
{
  EVA_DEBUG_LOG_PACKET_INIT = 0x542134a,
  EVA_DEBUG_LOG_PACKET_MAP,
  EVA_DEBUG_LOG_PACKET_MALLOC,
  EVA_DEBUG_LOG_PACKET_FREE,
  EVA_DEBUG_LOG_PACKET_REALLOC,
  EVA_DEBUG_LOG_PACKET_TIME
} GskDebugLogPacketType;


typedef struct
{
  GskDebugLogPacketType type;
  union {
    struct {
      guint n_bytes, n_contexts;
      guint64 *contexts;
      guint64 allocation;
    } malloc;
    struct {
      guint n_bytes, n_contexts;
      guint64 *contexts;
      guint64 allocation;
    } free;
    struct {
      guint64 mem;
      guint64 size;
    } realloc;
  } info;
} GskDebugLogPacket;

GskDebugLog       *eva_debug_log_open (const char *filename,
                                       GError    **error);
GskDebugLogPacket *eva_debug_log_read (GskDebugLog *log);
void               eva_debug_log_packet_free (GskDebugLogPacket *);
void               eva_debug_log_rewind (GskDebugLog *log);
void               eva_debug_log_close(GskDebugLog *);


G_END_DECLS

#endif
