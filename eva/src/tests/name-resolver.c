#include <stdio.h>
#include <stdlib.h>
#include "../evanameresolver.h"
#include "../evadebug.h"
#include "../evainit.h"
#include "../evamain.h"

static guint num_tasks_pending = 0;

static void
handle_destroy (gpointer data)
{
  (void) data;
  if (--num_tasks_pending == 0)
    eva_main_quit ();
}

static void
got_name_successfully (GskSocketAddress *address,
		       gpointer          unused)
{
  char *str = eva_socket_address_to_string (address);
  g_assert (unused == NULL);
  g_print ("%s\n", str);
  g_free (str);
}

static void
got_error (GError *error,
	   gpointer unused)
{
  g_assert (unused == NULL);
  g_printerr ("got_error: %s\n", error->message);
  eva_main_exit (1);
}

int main (int argc, char **argv)
{
  GskNameResolverFamily family;
  GskNameResolverTask *task;
  guint i;
  eva_init_without_threads (&argc, &argv);

  if (argc != 3)
    g_error ("%s requires exactly 2 arguments, family and name", argv[0]);

#define N_REPETITIONS   1
  num_tasks_pending = N_REPETITIONS;
  for (i = 0; i < N_REPETITIONS; ++i)
    {
      family = eva_name_resolver_family_get_by_name (argv[1]);
      if (!family)
	g_error ("invalid name-resolver family '%s'", argv[1]);

      task = eva_name_resolve (family,
			       argv[2],
			       got_name_successfully,
			       got_error,
			       NULL,
			       handle_destroy);
      eva_name_resolver_task_unref (task);
    }
  return eva_main_run ();
}
