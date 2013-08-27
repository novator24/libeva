#ifndef __EVA_STREAM_EXTERNAL_H_
#define __EVA_STREAM_EXTERNAL_H_

#include "evastream.h"
#include "evamainloop.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _EvaStreamExternal EvaStreamExternal;
typedef struct _EvaStreamExternalClass EvaStreamExternalClass;
/* --- type macros --- */
GType eva_stream_external_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_STREAM_EXTERNAL			(eva_stream_external_get_type ())
#define EVA_STREAM_EXTERNAL(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_STREAM_EXTERNAL, EvaStreamExternal))
#define EVA_STREAM_EXTERNAL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_STREAM_EXTERNAL, EvaStreamExternalClass))
#define EVA_STREAM_EXTERNAL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_STREAM_EXTERNAL, EvaStreamExternalClass))
#define EVA_IS_STREAM_EXTERNAL(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_STREAM_EXTERNAL))
#define EVA_IS_STREAM_EXTERNAL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_STREAM_EXTERNAL))

/* --- callbacks --- */
typedef void (*EvaStreamExternalTerminated) (EvaStreamExternal   *external,
					     EvaMainLoopWaitInfo *wait_info,
					     gpointer             user_data);
typedef void (*EvaStreamExternalStderr)     (EvaStreamExternal   *external,
					     const char          *error_text,
					     gpointer             user_data);

/* --- structures --- */

struct _EvaStreamExternalClass 
{
  EvaStreamClass stream_class;
};
struct _EvaStreamExternal 
{
  EvaStream      stream;

  /* stdin for the process */
  int write_fd;
  EvaSource *write_source;
  EvaBuffer write_buffer;
  gsize max_write_buffer;

  /* stdout for the process */
  int read_fd;
  EvaSource *read_source;
  EvaBuffer read_buffer;
  gsize max_read_buffer;

  /* stderr for the process */
  int read_err_fd;
  EvaSource *read_err_source;
  EvaBuffer read_err_buffer;
  gsize max_err_line_length;

  /* process-termination notification */
  EvaSource *process_source;
  glong pid;

  /* user-callback information */
  EvaStreamExternalTerminated term_func;
  EvaStreamExternalStderr err_func;
  gpointer user_data;
};

typedef enum
{
  EVA_STREAM_EXTERNAL_ALLOCATE_PSEUDOTTY = (1<<2),
  EVA_STREAM_EXTERNAL_SEARCH_PATH        = (1<<3)
} EvaStreamExternalFlags;
 


/* --- prototypes --- */
EvaStream *eva_stream_external_new       (EvaStreamExternalFlags      flags,
					  const char                 *stdin_filename,
					  const char                 *stdout_filename,
				          EvaStreamExternalTerminated term_func,
					  EvaStreamExternalStderr     err_func,
				          gpointer                    user_data,
				          const char                 *path,
				          const char                 *argv[],
				          const char                 *env[],
					  GError                    **error);

G_END_DECLS

#endif
