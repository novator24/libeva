#ifndef __EVA_LOG_H_
#define __EVA_LOG_H_

#include <glib.h>
#include "evalogringbuffer.h"

G_BEGIN_DECLS

typedef struct _EvaLogTrap EvaLogTrap;

/* Helper macros for using the gmessage system
   with many log-domains. */


/* Suppress warnings when GCC is in -pedantic mode and not -std=c99
 */
#if (__GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96))
#pragma GCC system_header
#endif


/* you must call this if you want use
   the other functions in this file (ie eva_log_trap_domain_to_file())
 */
void eva_log_init (void);

/* output_file_template may contain any format specifier
   as known by strftime. */
void eva_log_rotate_stdio_logs (const char *output_file_template,
                                gboolean    use_localtime,
                                guint       rotation_period);


/* output_format may contain the following escapes:
      %{message}       the message itself.
      %{NNNmessage}    the first NNN characters of message.
      %{localtime:FMT} the time/date in local timezone, formatted as per FMT.
      %{gmtime:FMT}    the time/date in gm, formatted as per FMT.
      %{domain}        the log domain.
      %{level}         the log level, as 'error', 'message', etc.
      %{glevel}        approximately how glib does the level:
                       'Debug', 'Info', '*Message*', '***Warning***',
                       '***Critical***', '***ERROR***'.
      %{Level}, %{LEVEL}  like %{level} with casing differences.
      %{levelsuffix}   '.', '!', '!!!' depending on the severity.
      %%               a percent symbol.
 */
EvaLogTrap *eva_log_trap_domain_to_file (const char *domain,
                                         GLogLevelFlags level_mask,
                                         const char *filename,
                                         const char *output_format);

typedef void (*EvaLogTrapFunc) (const char *domain,
                                GLogLevelFlags level,
                                const char *raw_message,
                                const char *formatted_message,
                                gpointer    data);

EvaLogTrap *eva_log_trap_generic      (const char    *domain,
                                       GLogLevelFlags trap_mask,
                                       const char    *output_format,
                                       EvaLogTrapFunc func,
                                       gpointer       data,
                                       GDestroyNotify destroy);
EvaLogTrap *eva_log_trap_ring_buffer  (const char    *domain,
                                       GLogLevelFlags trap_mask,
                                       EvaLogRingBuffer *buffer,
                                       const char       *output_format);
EvaLogTrap *eva_log_trap_ignore       (const char    *domain,
                                       GLogLevelFlags trap_mask);

/* indicate that the given logfile should
   be appended to, rather than overwritten.
   must be given before any other references to the logfile */
void eva_log_append (const char *filename);

#define EVA_LOG_DEFAULT_OUTPUT_FORMAT   \
        "%{localtime} %{Level}: [%{domain}]: %{200message}."

#ifdef G_HAVE_ISO_VARARGS
#define eva_error(domain, ...)    g_log (domain,               \
                                         G_LOG_LEVEL_ERROR,    \
                                         __VA_ARGS__)
#define eva_message(domain, ...)  g_log (domain,               \
                                         G_LOG_LEVEL_MESSAGE,  \
                                         __VA_ARGS__)
#define eva_critical(domain, ...) g_log (domain,               \
                                         G_LOG_LEVEL_CRITICAL, \
                                         __VA_ARGS__)
#define eva_warning(domain, ...)  g_log (domain,               \
                                         G_LOG_LEVEL_WARNING,  \
                                         __VA_ARGS__)
#define eva_debug(domain, ...)    g_log (domain,               \
                                         G_LOG_LEVEL_DEBUG,    \
                                         __VA_ARGS__)
#elif defined(G_HAVE_GNUC_VARARGS)
#define eva_error(domain, format...)      g_log (domain,               \
                                                 G_LOG_LEVEL_ERROR,    \
                                                 format)
#define eva_message(domain, format...)    g_log (domain,               \
                                                 G_LOG_LEVEL_MESSAGE,  \
                                                 format)
#define eva_critical(domain, format...)   g_log (domain,               \
                                                 G_LOG_LEVEL_CRITICAL, \
                                                 format)
#define eva_warning(domain, format...)    g_log (domain,               \
                                                 G_LOG_LEVEL_WARNING,  \
                                                 format)
#define eva_debug(domain, format...)      g_log (domain,               \
                                                 G_LOG_LEVEL_DEBUG,    \
                                                 format)
#else   /* no varargs macros */
static void
eva_error (const gchar *domain,
           const gchar *format,
           ...)
{
  va_list args;
  va_start (args, format);
  g_logv (domain, G_LOG_LEVEL_ERROR, format, args);
  va_end (args);
}
static void
eva_message (const gchar *domain,
             const gchar *format,
             ...)
{
  va_list args;
  va_start (args, format);
  g_logv (domain, G_LOG_LEVEL_MESSAGE, format, args);
  va_end (args);
}
static void
eva_critical (const gchar *domain,
              const gchar *format,
              ...)
{
  va_list args;
  va_start (args, format);
  g_logv (domain, G_LOG_LEVEL_CRITICAL, format, args);
  va_end (args);
}
static void
eva_warning (const gchar *domain,
             const gchar *format,
             ...)
{
  va_list args;
  va_start (args, format);
  g_logv (domain, G_LOG_LEVEL_WARNING, format, args);
  va_end (args);
}
static void
eva_debug (const gchar *domain,
           const gchar *format,
           ...)
{
  va_list args;
  va_start (args, format);
  g_logv (domain, G_LOG_LEVEL_DEBUG, format, args);
  va_end (args);
}
#endif  /* !__GNUC__ */

G_END_DECLS

#endif
