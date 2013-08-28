#include "../control/evacontrolserver.h"
#include "../evamainloop.h"
#include "../evainit.h"
#include "../evamain.h"
#include "../evabufferstream.h"
#include "../evalog.h"
#include "../hash/evahash.h"
#include <stdlib.h>
#include <string.h>

EvaSource *timeout_source = NULL;
char *text_to_print = NULL;
guint period = 1000;
guint count = 0;

static gboolean
handle_timeout (gpointer data)
{
  g_print ("%s\n", text_to_print);
  eva_message("testdomain", "count=%u", count++);
  return TRUE;
}

static gboolean
command_handler__period (char **argv,
                         EvaStream *input,
                         EvaStream **output,
                         gpointer data,
                         GError **error)
{
  char *end;
  guint per;
  if (argv[1] == NULL || argv[2] != NULL)
    {
      g_set_error (error, EVA_G_ERROR_DOMAIN,
                   EVA_ERROR_INVALID_ARGUMENT,
                   "period command expected exactly 1 argument, the milliseconds");
      return FALSE;
    }
  per= strtoul (argv[1], &end, 10);
  if (end == argv[1] || *end != 0)
    {
      g_set_error (error, EVA_G_ERROR_DOMAIN,
                   EVA_ERROR_INVALID_ARGUMENT,
                   "period command's argument was not number");
      return FALSE;
    }
  eva_source_adjust_timer (timeout_source, per, per);
  period = per;
  return TRUE;
}

static gboolean
command_handler__settext (char **argv,
                         EvaStream *input,
                         EvaStream **output,
                         gpointer data,
                         GError **error)
{
  if (argv[1] == NULL || argv[2] != NULL)
    {
      g_set_error (error, EVA_G_ERROR_DOMAIN,
                   EVA_ERROR_INVALID_ARGUMENT,
                   "settext command expected exactly 1 argument, the milliseconds");
      return FALSE;
    }
  g_free (text_to_print);
  text_to_print = g_strdup (argv[1]);
  return TRUE;
}

static gboolean handle_bs_timer (gpointer data)
{
  EvaBufferStream *bs = EVA_BUFFER_STREAM (data);
  eva_buffer_append_string (&bs->read_buffer, "... mom?!?\n");
  eva_buffer_stream_read_buffer_changed (bs);
  eva_buffer_stream_read_shutdown (bs);
  return FALSE;
}

static gboolean
command_handler__waittest (char **argv,
                           EvaStream *input,
                           EvaStream **output,
                           gpointer data,
                           GError **error)
{
  EvaBufferStream *bs = eva_buffer_stream_new ();
  if (!eva_io_write_shutdown (EVA_IO (bs), error))
    return FALSE;       /* shouldn't happen! */
  eva_buffer_append_string (&bs->read_buffer, "hi ...\n");
  eva_buffer_stream_read_buffer_changed (bs);
  eva_main_loop_add_timer (eva_main_loop_default (),
                           handle_bs_timer,
                           g_object_ref (bs),
                           g_object_unref,
                           1000, -1);
  *output = EVA_STREAM(bs);
  return TRUE;
}
typedef struct _MD5SUMData MD5SUMData;
struct _MD5SUMData
{
  EvaHash *hash;
  EvaBufferStream *bs;
  EvaStream *stream;
};
/* takes ownership of arguments */
static MD5SUMData *
md5sum_data_new (EvaBufferStream *bs, EvaHash *hash, EvaStream *input_stream)
{
  MD5SUMData *data = g_new (MD5SUMData, 1);
  data->bs = bs;
  data->hash = hash;
  data->stream = input_stream;
  return data;
}
static void
md5sum_data_free (MD5SUMData *data)
{
  eva_hash_destroy (data->hash);
  //g_object_unref (data->bs);
  g_object_unref (data->stream);
  g_free (data);
}

static gboolean
handle_input_stream_readable (EvaStream *stream, gpointer data)
{
  MD5SUMData *d = data;
  char buf[4096];
  GError *error = NULL;
  guint n_read = eva_stream_read (stream, buf, sizeof(buf), &error);
  if (error)
    g_error_free (error);
  eva_hash_feed (d->hash, buf, n_read);
  return TRUE;
}
static gboolean
handle_input_stream_read_shutdown (EvaStream *stream, gpointer data)
{
  MD5SUMData *d = data;
  char *hexbuf = g_alloca (eva_hash_get_size (d->hash) * 2 + 1);
  eva_hash_done (d->hash);
  eva_hash_get_hex (d->hash, hexbuf);
  eva_buffer_append_string (eva_buffer_stream_peek_read_buffer (d->bs), hexbuf);
  eva_buffer_append_string (eva_buffer_stream_peek_read_buffer (d->bs), "\n");
  //g_message ("write md5: %s",hexbuf);
  eva_buffer_stream_read_buffer_changed (d->bs);
  eva_buffer_stream_read_shutdown (d->bs);
  return FALSE;
}

static gboolean
command_handler__md5sum (char **argv,
                           EvaStream *input,
                           EvaStream **output,
                           gpointer data,
                           GError **error)
{
  EvaBufferStream *bs;
  if (input == NULL)
    {
      g_set_error (error, EVA_G_ERROR_DOMAIN,
                   EVA_ERROR_INVALID_ARGUMENT,
                   "md5sum requires post data");
      return FALSE;
    }
  bs = eva_buffer_stream_new ();
  *output = EVA_STREAM (bs);
  if (!eva_io_write_shutdown (EVA_IO (bs), error))
    return FALSE;       /* shouldn't happen! */
  eva_io_trap_readable (input, handle_input_stream_readable,
                        handle_input_stream_read_shutdown,
                        md5sum_data_new (bs, eva_hash_new_md5 (), g_object_ref (input)),
                        (GDestroyNotify) md5sum_data_free);
  return TRUE;
}

static void 
add_socket (EvaControlServer *server,
            const char       *path)
{
  EvaSocketAddress *addr = eva_socket_address_local_new (path);
  GError *error = NULL;
  g_assert (addr);
  if (!eva_control_server_listen (server, addr, &error))
    g_error ("error listening: %s", error->message);
  g_object_unref (addr);
}

static void usage ()
{
  g_print ("usage: %s --socket=SOCKET\n", g_get_prgname ());
  exit (1);
}
int main(int argc, char **argv)
{
  EvaControlServer *server;
  int i;
  gboolean got_socket = FALSE;

  eva_init_without_threads (&argc, &argv);
  g_set_prgname (argv[0]);
  
  server = eva_control_server_new ();

  for (i = 1; i < argc; i++)
    {
      if (strncmp (argv[i], "--socket=", 9) == 0)
        {
          add_socket (server, argv[i] + 9);
          got_socket = TRUE;
        }
      else if (strcmp (argv[i], "--socket") == 0)
        {
          add_socket (server, argv[++i]);
          got_socket = TRUE;
        }
      else
        usage ();
    }
  if (!got_socket)
    usage ();

  g_message ("about to eva_control_server_add_command");

  eva_control_server_add_command (server, "period", command_handler__period, NULL);
  eva_control_server_add_command (server, "settext", command_handler__settext, NULL);
  eva_control_server_add_command (server, "waittest", command_handler__waittest, NULL);
  eva_control_server_add_command (server, "md5sum", command_handler__md5sum, NULL);
  eva_control_server_set_file (server, "/data/letter",
                               (guint8 *) "hi mom!\n", 8, NULL);

  /* test deletion-- repeat to aid in finding memory leaks */
  for (i = 0; i <= 1000; i++)
    {
      eva_control_server_set_file (server, "/test-delete-file/letter",
                                   (guint8 *) "hi mom!\n", 8, NULL);
      eva_control_server_delete_file (server, "/test-delete-file/letter", NULL);
    }
  for (i = 0; i <= 1000; i++)
    {
      eva_control_server_set_file (server, "/test-delete/letter",
                                   (guint8 *) "hi mom!\n", 8, NULL);
      if (!eva_control_server_delete_directory (server, "/test-delete", NULL))
        g_error ("error deleting");
    }

  {
    GString *str = g_string_new ("");
    for (i = 0; i <= 1000; i++)
      g_string_append_printf (str, "line %u/%u: this is to make it possible to diagnose large-data problems\n", i,1000);
    eva_control_server_set_file (server, "/data/somewhat_big_file",
                                 (guint8 *) str->str, str->len, NULL);
  }
#define PROMPT  "ExampleServer:%n> "
  eva_control_server_set_file (server, "/server/client-prompt",
                               (guint8*) PROMPT, strlen(PROMPT), NULL);
  eva_control_server_set_logfile (server, "/logs/testdomain",
                                  2048,
                                  "testdomain", G_LOG_LEVEL_MASK,
                                  NULL);

  text_to_print = g_strdup ("hello world");
  g_message ("eva_main_loop_add_timer...");
  timeout_source = eva_main_loop_add_timer (eva_main_loop_default (),
                           handle_timeout, NULL, NULL,
                           period, period);
  g_message ("eva_main_loop_add_timer... done");

  return eva_main_run ();
}
