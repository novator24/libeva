#ifndef __EVA_PROCESS_INFO_H_
#define __EVA_PROCESS_INFO_H_

#include <glib.h>

typedef enum
{
  EVA_PROCESS_INFO_RUNNING = 'R',
  EVA_PROCESS_INFO_SLEEPING = 'S',
  EVA_PROCESS_INFO_DISK = 'D',
  EVA_PROCESS_INFO_ZOMBIE = 'Z',
  EVA_PROCESS_INFO_TRACED = 'T',
  EVA_PROCESS_INFO_PAGING = 'W'
} EvaProcessInfoState;
const char *eva_process_info_state_name (EvaProcessInfoState);

typedef struct _EvaProcessInfo EvaProcessInfo;
struct _EvaProcessInfo
{
  guint process_id;
  guint parent_process_id;
  guint process_group_id;
  guint session_id;
  guint tty_id;
  gulong user_runtime;
  gulong kernel_runtime;
  gulong child_user_runtime;		/* only includes reaped children */
  gulong child_kernel_runtime;		/* only includes reaped children */
  guint nice;
  EvaProcessInfoState state;
  gulong virtual_memory_size;
  gulong resident_memory_size;
  char *exe_filename;
};

EvaProcessInfo *eva_process_info_get  (guint           pid,
                                       GError        **error);
void             eva_process_info_free(EvaProcessInfo *info);

#endif
