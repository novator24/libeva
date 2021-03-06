#include "../evanameresolver.h"
#include "../evastdio.h"
#include "../evamain.h"
#include "../evainit.h"
#include "../evamainloop.h"
#include <stdlib.h>
#include <string.h>

static FILE *fp = NULL;
static guint n_pending = 0;

static guint
reduce_n_pending ()
{
  g_assert (n_pending > 0);
  if (--n_pending == 0 && fp == NULL)
    eva_main_quit ();
  return n_pending;
}

static void
handle_successful_name_resolution (EvaSocketAddress *address,
				    gpointer          func_data)
{
  char *str = eva_socket_address_to_string (address);
  const char *name = func_data;
  g_printerr ("%s => %s [npending=%u]\n", name, str, reduce_n_pending());
  g_free (str);
  g_free (func_data);
}
static void
handle_failed_name_resolution (GError           *error,
			       gpointer          func_data)
{
  const char *name = func_data;
  g_printerr ("%s FAILED: %s [npending=%u]\n", name, error->message, reduce_n_pending());
  g_free (func_data);
}

static gboolean
do_lookup_and_schedule_next (gpointer data)
{
  EvaNameResolverTask *task;
  char *line = eva_stdio_readline (fp);
  char *tab;
  const char *name;
  int delay;
  if (line == NULL)
    {
      fclose (fp);
      fp = NULL;
      if (n_pending == 0)
	eva_main_quit ();
      return FALSE;
    }
  if (*line == 0)
    {
      g_warning ("huh");
      fclose (fp);
      g_free(line);
      fp = NULL;
      if (n_pending == 0)
	eva_main_quit ();
      return FALSE;
    }
  n_pending++;
  tab = strchr (line, '\t');
  g_assert(tab);
  *tab = 0;
  name = tab + 1;
  task = eva_name_resolve (EVA_NAME_RESOLVER_FAMILY_IPV4,
			   name,
			   handle_successful_name_resolution,
			   handle_failed_name_resolution,
			   g_strdup (name),
			   NULL);
  g_assert (task);
  eva_name_resolver_task_unref (task);
  delay = atoi (line);
  g_free (line);

  if (delay)
    eva_main_loop_add_timer (eva_main_loop_default (), do_lookup_and_schedule_next, NULL, NULL, delay, -1);
  else
    eva_main_loop_add_idle (eva_main_loop_default (), do_lookup_and_schedule_next, NULL, NULL);

  return FALSE;
}


int main(int argc, char **argv)
{
  eva_init_without_threads (&argc, &argv);
  if (argc != 2)
    g_error ("usage: %s SCRIPT", argv[0]);
  fp = fopen (argv[1], "r");
  g_assert (fp);
  do_lookup_and_schedule_next (NULL);
  return eva_main_run();
}
