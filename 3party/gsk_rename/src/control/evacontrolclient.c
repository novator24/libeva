#include "evacontrolclient.h"
#include "../http/evahttpclient.h"
#include "../evastreamclient.h"
#include "../evamemory.h"
#include "../evastdio.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

struct _EvaControlClient
{
  EvaSocketAddress *address;

  char *prompt;
  guint cmd_no;

  guint add_newlines_as_needed : 1;

  EvaControlClientErrorFunc error_func;
  gpointer error_func_data;

  int last_argc;
  char **last_argv;
};

static void
abort_on_error (GError *error, gpointer data)
{
  EvaControlClient *cc = data;
  g_error ("control-client error: %s: %s",
           eva_socket_address_to_string (cc->address),
           error->message);
}

/**
 * eva_control_client_new:
 * @server: the location of the server to contact.
 *
 * Allocates a new control client.
 *
 * This may invoke the main-loop to answer a few questions.
 *
 * returns: the newly allocated control client.
 */
EvaControlClient *
eva_control_client_new (EvaSocketAddress *server)
{
  EvaControlClient *cc = g_new (EvaControlClient, 1);
  cc->address = server ? g_object_ref (server) : NULL;
  cc->prompt = NULL;
  cc->cmd_no = 1;
  cc->add_newlines_as_needed = 1;
  cc->error_func = abort_on_error;
  cc->error_func_data = cc;
  return cc;
}

/**
 * eva_control_client_set_flag:
 * @cc:
 * @flag:
 * @value:
 *
 * not used yet.
 */
void
eva_control_client_set_flag (EvaControlClient *cc,
                             EvaControlClientFlag flag,
                             gboolean             value)
{
  switch (flag)
    {
    case EVA_CONTROL_CLIENT_ADD_NEWLINES_AS_NEEDED:
      cc->add_newlines_as_needed = value ? 1 : 0;
      break;
    default:
      g_return_if_reached ();
    }
}

/**
 * eva_control_client_get_flag:
 * @cc:
 * @flag:
 * returns:
 *
 * not used yet.
 */
gboolean
eva_control_client_get_flag (EvaControlClient *cc,
                             EvaControlClientFlag flag)
{
  switch (flag)
    {
    case EVA_CONTROL_CLIENT_ADD_NEWLINES_AS_NEEDED:
      return cc->add_newlines_as_needed;
    default:
      g_return_val_if_reached (FALSE);
    }
}

/**
 * eva_control_client_set_prompt:
 * @cc: the control client to affect.
 * @prompt_format: format string.
 *
 * Set the prompt format string.
 */
void
eva_control_client_set_prompt(EvaControlClient *cc,
                              const char       *prompt_format)
{
  char *to_free = cc->prompt;
  cc->prompt = g_strdup (prompt_format);
  g_free (to_free);
}

typedef struct _GetServerFileStatus GetServerFileStatus;
struct _GetServerFileStatus
{
  gboolean done;
  GError *error;
  gpointer *contents_out;
  gsize *length_out;
};

static void
buffer_callback_get_server_file (EvaBuffer *buffer, gpointer data)
{
  GetServerFileStatus *fs = data;
  *(fs->length_out) = buffer->size;
  *(fs->contents_out) = g_malloc (buffer->size);
  eva_buffer_read (buffer, *(fs->contents_out), buffer->size);
  fs->done = TRUE;
}

static void
handle_get_server_file_response (EvaHttpRequest  *request,
                                 EvaHttpResponse *response,
                                 EvaStream       *input,
                                 gpointer         hook_data)
{
  GetServerFileStatus *fs = hook_data;
  EvaStream *stream;
  if (response->status_code != EVA_HTTP_STATUS_OK)
    {
      fs->error = g_error_new (EVA_G_ERROR_DOMAIN,
                               EVA_ERROR_HTTP_NOT_FOUND,
                               "got status code %u from server",
                               response->status_code);
      fs->done = TRUE;
      return;
    }
  stream = eva_memory_buffer_sink_new (buffer_callback_get_server_file, fs, NULL);
  eva_stream_attach (input, stream, NULL);
  g_object_unref (stream);
}

static gboolean
get_server_file (EvaControlClient *cc,
                 const char       *path,
                 gpointer         *contents_out,
                 gsize            *length_out,
                 GError          **error)
{
  EvaHttpRequest *request;
  GetServerFileStatus status = { FALSE, NULL, contents_out, length_out };
  EvaHttpClient *http_client;
  EvaStream *client_stream;
  char *p = g_strdup_printf ("/run.txt?command=cat&arg1=%s", path);
  http_client = eva_http_client_new ();
  request = eva_http_request_new (EVA_HTTP_VERB_GET, p);
  g_free (p);
  eva_http_client_request (http_client, request, NULL,
                           handle_get_server_file_response, &status,
                           NULL);

  client_stream = eva_stream_new_connecting (cc->address, error);
  if (client_stream == NULL)
    return FALSE;
  if (!eva_stream_attach_pair (EVA_STREAM (http_client), client_stream, error))
    return FALSE;
  g_object_unref (request);
  g_object_unref (client_stream);
  g_object_unref (http_client);

  while (!status.done)
    eva_main_loop_run (eva_main_loop_default (), -1, NULL);
  if (status.error)
    {
      *error = status.error;
      return FALSE;
    }
  return TRUE;
}

/**
 * eva_control_client_get_prompt:
 * @cc: the control client whose prompt should be obtained.
 * returns: the new prompt string.
 *
 * Get a newly allocated prompt string.
 * This is used by the user-interface program;
 * typically readline() is employed to use this prompt.
 */
char *
eva_control_client_get_prompt_string(EvaControlClient *cc)
{
  GString *rv;
  guint i;
  if (cc->prompt == NULL)
    {
      /* request prompt string from server. */
      gpointer content; gsize len;
      GError *error = NULL;
      if (get_server_file (cc, "/server/client-prompt", &content, &len, &error))
        {
          cc->prompt = g_strndup (content, len);
          g_free (content);
        }
      else
        {
          g_message ("error: %s", error->message);
          g_error_free (error);
          cc->prompt = g_strdup ("");
        }
    }
  rv = g_string_new ("");
  for (i = 0; cc->prompt[i] != 0; i++)
    {
      if (cc->prompt[i] == '%')
        {
          switch (cc->prompt[++i])
            {
            case 'n':
              g_string_append_printf (rv, "%u", cc->cmd_no);
              break;
            case '%':
              g_string_append_c (rv, '%');
              break;
            default:
              /* discard */
              g_warning ("bad prompt string '%s'", cc->prompt);
              break;
            }
        }
      else
        g_string_append_c (rv, cc->prompt[i]);
    }
  return g_string_free (rv, FALSE);
}

static void
set_server_address (EvaControlClient *client,
                    const char       *path)
{
  if (client->address)
    {
      g_warning ("already had address");
      return;
    }
  client->address = eva_socket_address_local_new (path);
}

static void
run_script (EvaControlClient *cc,
            const char       *filename)
{
  FILE *fp = fopen (filename, "r");
  guint old_cmd_no;
  char *line;
  if (fp == NULL)
    {
      GError *error = g_error_new (EVA_G_ERROR_DOMAIN,
                                   eva_error_code_from_errno (errno),
                                   "error opening file: %s",
                                   g_strerror (errno));
      if (cc->error_func)
        cc->error_func (error, cc->error_func_data);
      g_error_free (error);
      return;
    }
  old_cmd_no = cc->cmd_no;
  while ((line=eva_stdio_readline (fp)) != NULL)
    {
      g_strstrip (line);
      if (line[0] == 0 || line[0] == '#')
        {
          g_free (line);
          continue;
        }
      eva_control_client_run_command_line (cc, line);
      g_free (line);
    }
  cc->cmd_no = old_cmd_no;
  fclose (fp);
}

#define TEST_OPTION_FLAG(flags, opt) \
  ((flags&EVA_CONTROL_CLIENT_OPTIONS_##opt) == EVA_CONTROL_CLIENT_OPTIONS_##opt)
/**
 * eva_control_client_parse_command_line_args:
 * @cc: the control client to affect.
 * @argc_inout: a reference to the 'argc' which was passed into main.
 * @argv_inout: a reference to the 'argv' which was passed into main.
 * @flags: bitwise-OR'd flags telling which command-line arguments to parse.
 * returns: whether to parse commands from stdin.
 *
 * Parse standard command-line options.
 *
 * During this parsing, some remote commands may be run.
 * For example -e flags cause instructions to be executed.
 * Therefore, this may reinvoke the mainloop.
 */
gboolean
eva_control_client_parse_command_line_args (EvaControlClient *cc,
                                            int              *argc_inout,
                                            char           ***argv_inout,
                                            EvaControlClientOptionFlags flags)
{
  int i;
  gboolean user_specified_interactive = FALSE;
  gboolean ran_commands = FALSE;
#define SWALLOW_ARG(count)              \
    { memmove ((*argv_inout) + i,       \
               (*argv_inout) + (i + count), \
               sizeof(char*) * (*argc_inout + 1 - (i + count))); \
      *argc_inout -= count; }

  for (i = 1; i < *argc_inout; )
    {
      const char *arg = (*argv_inout)[i];
      if (TEST_OPTION_FLAG (flags, RANDOM))
        {
          if (strcmp (arg, "--exact-newlines") == 0)
            {
              SWALLOW_ARG(1);
              cc->add_newlines_as_needed = 0;
              continue;
            }
        }
      if (TEST_OPTION_FLAG (flags, INTERACTIVE))
        {
          if (strcmp (arg, "-i") == 0
           || strcmp (arg, "--interactive") == 0)
            {
              user_specified_interactive = TRUE;
              continue;
            }
        }
      if (TEST_OPTION_FLAG (flags, INLINE_COMMAND))
        {
          if (strcmp (arg, "-e") == 0)
            {
              const char *cmd = (*argv_inout)[i+1];
              SWALLOW_ARG (2);
              eva_control_client_run_command_line (cc, cmd);
              ran_commands = TRUE;
              continue;
            }
        }
      if (TEST_OPTION_FLAG (flags, SOCKET))
        {
          if (strcmp (arg, "--socket") == 0)
            {
              const char *path = (*argv_inout)[i+1];
              SWALLOW_ARG (2);
              set_server_address (cc, path);
              continue;
            }
          if (g_str_has_prefix (arg, "--socket="))
            {
              const char *path = strchr (arg, '=') + 1;
              SWALLOW_ARG (1);
              set_server_address (cc, path);
              continue;
            }
        }
      if (TEST_OPTION_FLAG (flags, SCRIPTS))
        {
          if (strcmp (arg, "-f") == 0)
            {
              const char *file = (*argv_inout)[i+1];
              SWALLOW_ARG (2);
              run_script (cc, file);
              ran_commands = TRUE;
              continue;
            }
        }

      i++;
    }

#undef SWALLOW_ARG
  return !ran_commands || user_specified_interactive;
}

/**
 * eva_control_client_print_command_line_usage:
 * @flags: bitwise-OR'd flags telling which command-line arguments to parse.
 *
 * Prints the command-line usage that the control-client class defines.
 */
void
eva_control_client_print_command_line_usage(EvaControlClientOptionFlags flags)
{
  if (TEST_OPTION_FLAG (flags, RANDOM))
    {
      g_print ("  --exact-newlines      Don't add newlines to output.\n");
    }
  if (TEST_OPTION_FLAG (flags, INTERACTIVE))
    {
      g_print ("  -i, --interactive     Force interaction.\n");
    }
  if (TEST_OPTION_FLAG (flags, INLINE_COMMAND))
    {
      g_print ("  -e 'CMD'              Run the command CMD.\n");
    }
  if (TEST_OPTION_FLAG (flags, SCRIPTS))
    {
      g_print ("  -f 'SCRIPT'           Run commands from the file SCRIPT.\n");
    }
  if (TEST_OPTION_FLAG (flags, SOCKET))
    {
      g_print ("  --socket=SOCKET       Connect to the server on the given\n"
               "                        unix-domain socket.\n");
    }
}
#undef TEST_OPTION_FLAG

gboolean eva_control_client_has_address (EvaControlClient *client)
{
  return client->address != NULL;
}

void
eva_control_client_run_command_line (EvaControlClient *cc,
                                     const char       *line)
{
  int argc;
  char **argv;
  GError *error = NULL;
  char *infile = NULL, *outfile = NULL;
  if (!g_shell_parse_argv (line, &argc, &argv, &error))
    {
      g_warning ("error parsing command-line: %s", error->message);
      g_error_free (error);
      return;
    }
  while (argc >= 3)
    {
      if (strcmp (argv[argc - 2], "<") == 0)
        {
          g_free (argv[argc - 2]);
          g_free (infile);
          argv[argc - 2] = NULL;
          infile = argv[argc - 1];
          argc -= 2;
        }
      else if (strcmp (argv[argc - 2], ">") == 0)
        {
          g_free (argv[argc - 2]);
          g_free (outfile);
          argv[argc - 2] = NULL;
          outfile = argv[argc - 1];
          argc -= 2;
        }
      else
        break;
    }
  eva_control_client_run_command (cc, argv, infile, outfile);
  g_free (infile);
  g_free (outfile);
  g_strfreev (argv);
  return;
}

typedef struct _RequestInfo RequestInfo;
struct _RequestInfo
{
  EvaStream *output;
  gboolean output_finalized;
};

static void
handle_output_finalized (RequestInfo *ri)
{
  ri->output_finalized = TRUE;
}

static void
handle_response (EvaHttpRequest  *request,
                 EvaHttpResponse *response,
                 EvaStream       *input,
                 gpointer         hook_data)
{
  RequestInfo *ri = hook_data;
  EvaStream *out = EVA_STREAM (ri->output);
#if 0
  if (response->status_code != EVA_HTTP_STATUS_OK)
    g_warning ("ERROR response from server");
#endif

  /* TODO: this doesn't add a newline if needed... */
  eva_stream_attach (input, out, NULL);

  g_object_weak_ref (G_OBJECT (out), (GWeakNotify) handle_output_finalized, ri);
}

static void
request_info_unref_output_stream (gpointer ri)
{
  RequestInfo *request_info = ri;
  g_object_unref (request_info->output);
}

static void
append_url_quoted (GString *str, const char *t)
{
  while (*t)
    {
      const char *end = t;
      while (('a' <= *end && *end <= 'z')
          || ('A' <= *end && *end <= 'Z')
          || ('0' <= *end && *end <= '9')
          || *end == '-' || *end == '_' || *end == '/')
        end++;
      if (end > t)
        g_string_append_len (str, t, end - t);
      t = end;
      if (*t == 0)
        break;
      g_string_append_printf (str, "%%%02X", (guint8)*t);
      t++;
    }
}


void
eva_control_client_run_command (EvaControlClient *client,
                                char **command_and_args,
                                const char *in_filename,
                                const char *out_filename)
{
  EvaStream *client_stream;
  GError *error = NULL;
  EvaStream *in_stream, *out_stream;
  EvaHttpClient *http_client;
  GString *path;
  guint i;
  EvaHttpRequest *request;
  RequestInfo request_info;

  client_stream = eva_stream_new_connecting (client->address, &error);
  if (client_stream == NULL)
    {
      if (client->error_func)
        (*client->error_func) (error, client->error_func_data);
      g_error_free (error);
      return;
    }
  http_client = eva_http_client_new ();
  if (!eva_stream_attach_pair (EVA_STREAM (http_client), client_stream, &error))
    {
      if (client->error_func)
        (*client->error_func) (error, client->error_func_data);
      g_error_free (error);
      return;
    }

  path = g_string_new ("/run.txt?");
  g_string_append (path, "command=");
  append_url_quoted (path, command_and_args[0]);

  for (i = 1; command_and_args[i]; i++)
    {
      char buf[256];
      g_string_append_c (path, '&');
      g_snprintf (buf, sizeof (buf), "arg%u", i);
      g_string_append (path, buf);
      g_string_append_c (path, '=');
      append_url_quoted (path, command_and_args[i]);
    }
  client->cmd_no++;

  request = eva_http_request_new (in_filename ? EVA_HTTP_VERB_POST : EVA_HTTP_VERB_GET, path->str);
  if (in_filename)
    EVA_HTTP_HEADER (request)->transfer_encoding_type = EVA_HTTP_TRANSFER_ENCODING_CHUNKED;
  else
    EVA_HTTP_HEADER (request)->connection_type = EVA_HTTP_CONNECTION_CLOSE; /* HACK */
  g_string_free (path, TRUE);
  if (in_filename)
    {
      in_stream = eva_stream_fd_new_read_file (in_filename, &error);
      if (in_stream == NULL)
        {
          if (client->error_func)
            (*client->error_func) (error, client->error_func_data);
          g_error_free (error);
          return;
        }
    }
  else
    in_stream = NULL;
  if (out_filename)
    {
      out_stream = eva_stream_fd_new_write_file (out_filename, TRUE, TRUE, &error);
      if (out_stream == NULL)
        {
          if (client->error_func)
            (*client->error_func) (error, client->error_func_data);
          g_error_free (error);
          return;
        }
    }
  else
    {
      int fd = dup (STDOUT_FILENO);
      if (fd < 0)
        {
          g_error ("error running dup(1)");
        }
      out_stream = eva_stream_fd_new_auto (fd);
    }
  request_info.output = out_stream;
  request_info.output_finalized = FALSE;
  eva_http_client_request (http_client, request, in_stream,
                           handle_response, &request_info,
                           request_info_unref_output_stream);
  eva_http_client_shutdown_when_done (http_client);
  g_object_unref (http_client);
  g_object_unref (client_stream);
  if (in_stream)
    g_object_unref (in_stream);
  g_object_unref (request);

  while (!request_info.output_finalized)
    {
      eva_main_loop_run (eva_main_loop_default (), -1, NULL);
    }
}

void
eva_control_client_increment_command_number (EvaControlClient *cc)
{
  ++(cc->cmd_no);
}

void
eva_control_client_set_command_number (EvaControlClient *cc, guint no)
{
  cc->cmd_no = no;
}
