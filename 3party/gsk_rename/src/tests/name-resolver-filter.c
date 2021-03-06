#include <stdio.h>
#include <stdlib.h>
#include "../evanameresolver.h"
#include "../evadebug.h"
#include "../evainit.h"
#include "../evamain.h"

static EvaNameResolverFamily family;


static void next_name_resolution (void);

static void
got_name_successfully (EvaSocketAddress *address,
		       gpointer          unused)
{
  char *str = eva_socket_address_to_string (address);
  g_assert (unused == NULL);
  g_print ("%s\n", str);
  g_free (str);

  next_name_resolution ();
}

static void
got_error (GError *error,
	   gpointer unused)
{
  g_assert (unused == NULL);
  g_print ("failed: %s\n", error->message);
  fflush(stdout);
  
  next_name_resolution ();
}

static void
next_name_resolution (void)
{
  char buf[8192];
  static gboolean tried_next_name_resolution_recurse = FALSE;
  static gboolean next_name_resolution_running = FALSE;

  if (next_name_resolution_running)
    {
      tried_next_name_resolution_recurse = TRUE;
      return;
    }
  next_name_resolution_running = TRUE;
  do
    {
      tried_next_name_resolution_recurse = FALSE;
      if (!fgets (buf, sizeof (buf), stdin))
        eva_main_quit ();
      else
        {
          EvaNameResolverTask *task;
          g_strstrip (buf);
          g_printerr ("looking up '%s'\n", buf);
          task = eva_name_resolve (family, buf, got_name_successfully, got_error, NULL, NULL);
          eva_name_resolver_task_unref (task);
        }
    }
  while (tried_next_name_resolution_recurse);
  next_name_resolution_running = FALSE;
}

int main (int argc, char **argv)
{
  eva_init_without_threads (&argc, &argv);

  if (argc != 2)
    g_error ("%s requires exactly 1 argument, family", argv[0]);

  family = eva_name_resolver_family_get_by_name (argv[1]);
  if (!family)
    g_error ("invalid name-resolver family '%s'", argv[1]);

  next_name_resolution ();
  return eva_main_run ();
}
