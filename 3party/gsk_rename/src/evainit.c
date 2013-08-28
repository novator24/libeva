#include <string.h>
#include <signal.h>
#include "evainit.h"
#include "evaerror.h"
#include "evabuffer.h"
#include "evanameresolver.h"
#include "evamainloop.h"
#include "evadebug.h"

_EvaInitFlags eva_init_flags = 0;
gpointer eva_main_thread = NULL;

/**
 * eva_init_info_get_defaults:
 * @info: the #EvaInitInfo to fill.
 *
 * Obtain the default initialization information.
 * This should be run before eva_init() or eva_init_info_parse_args().
 *
 * This API has been deprecated for public use,
 * because it doesn't allow us to expand
 * EvaInitInfo without breaking binary-compatibility.
 *
 * Use eva_init_info_new() instead.
 */
void
eva_init_info_get_defaults (EvaInitInfo *info)
{
  info->prgname = NULL;
  info->needs_threads = 1;
}

/**
 * eva_init_info_new:
 *
 * Create a new, default initialization-configuration object.
 *
 * returns: the newly allocated #EvaInitInfo.
 */
EvaInitInfo *
eva_init_info_new (void)
{
  EvaInitInfo *info = g_new (EvaInitInfo, 1);
  eva_init_info_get_defaults (info);
  return info;
}

/**
 * eva_init_info_free:
 * @info: the object to free.
 *
 * Free a initialization-configuration object.
 */
void
eva_init_info_free (EvaInitInfo *info)
{
  g_free (info);
}

/**
 * eva_init:
 * @argc: a reference to main()'s argc;
 * this will be decreased if arguments are parsed
 * out of the argument array.
 * @argv: a reference to main()'s argc;
 * this may have arguments removed.
 * @info: the #EvaInitInfo to use as hints,
 * which will be filled with the
 * actual initialization information used.
 * If NULL, default initialization parameters
 * will be used.
 *
 * Initialize the EVA library.
 */
void
eva_init                   (int         *argc,
			    char      ***argv,
			    EvaInitInfo *info)
{
  g_type_init ();
  if (info == NULL)
    {
      info = g_newa (EvaInitInfo, 1);
      eva_init_info_get_defaults (info);
    }
  eva_init_info_parse_args (info, argc, argv);
  eva_init_raw (info);
}

/**
 * eva_init_without_threads:
 * @argc: a reference to main()'s argc;
 * this will be decreased if arguments are parsed
 * out of the argument array.
 * @argv: a reference to main()'s argc;
 * this may have arguments removed.
 *
 * Initialize the EVA library indicating that you will not use threads.
 */
void
eva_init_without_threads   (int         *argc,
			    char      ***argv)
{
  EvaInitInfo info;
  g_type_init ();
  eva_init_info_get_defaults (&info);
  info.needs_threads = FALSE;
  eva_init_info_parse_args (&info, argc, argv);
  eva_init_raw (&info);
}

static struct
{
  const char *name;
  guint flag;
} debug_flag_names[] = {
  { "io", EVA_DEBUG_IO },
  { "stream-data", EVA_DEBUG_STREAM_DATA },
  { "stream", EVA_DEBUG_STREAM },
  { "listener", EVA_DEBUG_STREAM_LISTENER },
  { "lifetime", EVA_DEBUG_LIFETIME },
  { "mainloop", EVA_DEBUG_MAIN_LOOP },
  { "dns", EVA_DEBUG_DNS },
  { "hook", EVA_DEBUG_HOOK },
  { "request", EVA_DEBUG_REQUEST },
  { "fd", EVA_DEBUG_FD },
  { "ssl", EVA_DEBUG_SSL },
  { "all", EVA_DEBUG_ALL },
  { NULL, 0 }
};

static void
handle_debug_flags (const char *opts)
{
  for (;;)
    {
      guint k;
      for (k = 0; debug_flag_names[k].name != NULL; k++)
	{
	  const char *o = debug_flag_names[k].name;
	  if (strncmp (opts, o, strlen (o)) == 0)
	    {
	      eva_debug_add_flags (debug_flag_names[k].flag);
	      break;
	    }
	}
      if (debug_flag_names[k].name == NULL)
	{
	  char *tmp = g_strdup (opts);
	  char *comma = strchr (tmp, ',');
	  if (comma)
	    *comma = 0;
	  g_warning ("no debugging option `%s' found", tmp);
	  g_free (tmp);
	}
      opts = strchr (opts, ',');
      if (!opts)
	break;
      opts++;
    }
}

/**
 * eva_init_info_parse_args:
 * @in_out: the #EvaInitInfo to fill.
 * @argc: the argument count (may be modified)
 * @argv: the arguments (may be modified)
 *
 * Parse/modify arguments and return their values in @in_out.
 *
 * The only currently supported argument is --eva-debug=FLAGS.
 */
void
eva_init_info_parse_args (EvaInitInfo *in_out,
			  int         *argc,
			  char      ***argv)
{
  gint i;
  g_type_init ();
  if (in_out->prgname == NULL && argv != NULL)
    in_out->prgname = (*argv)[0];

  for (i = 1; i < *argc; )
    {
      const char *arg = (*argv)[i];
      guint num_to_swallow = 0;

      /* handle --eva-debug= */
      if (strncmp (arg, "--eva-debug=", 12) == 0)
	{
	  const char *opts = arg + 12;
	  handle_debug_flags (opts);
	  num_to_swallow = 1;
	}

      if (strcmp (arg, "--g-fatal-warnings") == 0)
	{
	  g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);
	  num_to_swallow = 1;
	}

      if (num_to_swallow)
	{
	  memcpy ((*argv) + i, (*argv) + i + num_to_swallow,
		  (*argc - num_to_swallow - i + 1) * sizeof (char *));
	  *argc -= num_to_swallow;
	}
      else
	{
	  i++;
	}
    }

  {
    const char *debug_flags = g_getenv ("EVA_DEBUG");
    if (debug_flags)
      handle_debug_flags (debug_flags);
  }
}

static void
eva_socket_address_family_init (void)
{
  g_type_class_ref (EVA_TYPE_SOCKET_ADDRESS_IPV4);
  g_type_class_ref (EVA_TYPE_SOCKET_ADDRESS_IPV6);
}

void _eva_hook_init(void);
void _eva_name_resolver_init (void);
void _eva_url_transfer_register_builtins (void);

/**
 * eva_init_raw:
 * @info: information to use for initializing.
 *
 * Initialize EVA.
 */
void
eva_init_raw (EvaInitInfo *info)
{
  static gboolean has_initialized = FALSE;

  if (has_initialized)
    return;
  has_initialized = TRUE;

  if (info->prgname != NULL && g_get_prgname () == NULL)
    g_set_prgname (info->prgname);

  eva_init_flags = 0;
  if (info->needs_threads)
    eva_init_flags |= _EVA_INIT_SUPPORT_THREADS;

  g_type_init ();
  if (info->needs_threads)
    {
      g_thread_init (NULL);
      eva_main_thread = g_thread_self ();
    }
  _eva_hook_init ();
  _eva_error_init ();
  _eva_name_resolver_init ();
  _eva_main_loop_init ();
  _eva_url_transfer_register_builtins ();
  eva_socket_address_family_init ();

  /* we always want to ignore SIGPIPE;
     we just handle errno==EPIPE properly. */
  signal (SIGPIPE, SIG_IGN);
}
