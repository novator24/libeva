#ifndef __EVA_INIT_H_
#define __EVA_INIT_H_

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GskInitInfo GskInitInfo;
struct _GskInitInfo
{
  char *prgname;
  guint needs_threads : 1;
};

GskInitInfo *eva_init_info_new  (void);
void         eva_init_info_free (GskInitInfo *info);
void         eva_init           (int         *argc,
				 char      ***argv,
				 GskInitInfo *info);
void eva_init_without_threads   (int         *argc,
				 char      ***argv);

/* note: we *always* permit your programs to fork(),
   see evafork.h for caveats. */

#define eva_init_get_support_threads()	((eva_init_flags & _EVA_INIT_SUPPORT_THREADS) == _EVA_INIT_SUPPORT_THREADS)

#ifndef EVA_DISABLE_DEPRECATED
void eva_init_info_get_defaults (GskInitInfo *info);
#endif

/* --- implementation details --- */
void eva_init_info_parse_args   (GskInitInfo *in_out,
				 int         *argc,
				 char      ***argv);
void eva_init_raw               (GskInitInfo *info);

typedef enum
{
  _EVA_INIT_SUPPORT_THREADS = (1<<0)
} _GskInitFlags;
extern _GskInitFlags eva_init_flags;

extern gpointer eva_main_thread;

#define EVA_IS_MAIN_THREAD()		(eva_main_thread == NULL || (g_thread_self () == eva_main_thread))

G_END_DECLS

#endif
