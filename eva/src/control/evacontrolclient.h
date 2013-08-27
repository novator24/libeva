#ifndef __EVA_CONTROL_CLIENT_H_
#define __EVA_CONTROL_CLIENT_H_

/* talks to a special http server; reenters the main-loop */
typedef struct _EvaControlClient EvaControlClient;

#include "../evasocketaddress.h"

G_BEGIN_DECLS

EvaControlClient *
eva_control_client_new (EvaSocketAddress *server);

typedef enum
{
  EVA_CONTROL_CLIENT_ADD_NEWLINES_AS_NEEDED
} EvaControlClientFlag;

typedef enum
{
  EVA_CONTROL_CLIENT_OPTIONS_INTERACTIVE = (1<<0),      /* -i, --interactive */
  EVA_CONTROL_CLIENT_OPTIONS_RANDOM = (1<<1),           /* many misc options */
  EVA_CONTROL_CLIENT_OPTIONS_INLINE_COMMAND = (1<<2),   /* -e 'COMMAND' */
  EVA_CONTROL_CLIENT_OPTIONS_SOCKET = (1<<3),           /* --socket=SOCKET */
  EVA_CONTROL_CLIENT_OPTIONS_SCRIPTS = (1<<4),          /* -f SCRIPTFILE */

  EVA_CONTROL_CLIENT_OPTIONS_DEFAULT = 0xffffffff       /* all options by default */
} EvaControlClientOptionFlags;


typedef void (*EvaControlClientErrorFunc) (GError *error,
                                           gpointer data);

void
eva_control_client_set_flag (EvaControlClient *cc,
                             EvaControlClientFlag flag,
                             gboolean             value);

gboolean
eva_control_client_get_flag (EvaControlClient *cc,
                             EvaControlClientFlag flag);

void
eva_control_client_set_prompt(EvaControlClient *cc,
                              const char       *prompt_format);

/* may invoke the main-loop */
char *
eva_control_client_get_prompt_string(EvaControlClient *cc);

gboolean
eva_control_client_parse_command_line_args (EvaControlClient *cc,
                                            int              *argc_inout,
                                            char           ***argv_inout,
                                            EvaControlClientOptionFlags flags);
void
eva_control_client_print_command_line_usage(EvaControlClientOptionFlags flags);

gboolean eva_control_client_has_address (EvaControlClient *client);

void eva_control_client_increment_command_number (EvaControlClient *cc);
void eva_control_client_set_command_number (EvaControlClient *cc, guint no);

// invoke the mainloop
void
eva_control_client_run_command_line (EvaControlClient *client,
                                     const char       *line);
void
eva_control_client_run_command (EvaControlClient *,
                                char **command_and_args,
                                const char *input_file,
                                const char *output_file);

G_END_DECLS

#endif
