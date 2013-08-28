#ifndef __EVA_STREAM_FD_H_
#define __EVA_STREAM_FD_H_

#include "evastream.h"
#include "evasocketaddresssymbolic.h"

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _EvaStreamFd EvaStreamFd;
typedef struct _EvaStreamFdClass EvaStreamFdClass;

/* --- type macros --- */
GType eva_stream_fd_get_type(void) G_GNUC_CONST;
#define EVA_TYPE_STREAM_FD			(eva_stream_fd_get_type ())
#define EVA_STREAM_FD(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EVA_TYPE_STREAM_FD, EvaStreamFd))
#define EVA_STREAM_FD_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EVA_TYPE_STREAM_FD, EvaStreamFdClass))
#define EVA_STREAM_FD_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EVA_TYPE_STREAM_FD, EvaStreamFdClass))
#define EVA_IS_STREAM_FD(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EVA_TYPE_STREAM_FD))
#define EVA_IS_STREAM_FD_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EVA_TYPE_STREAM_FD))

#define EVA_STREAM_FD_GET_FD(stream)	(EVA_STREAM_FD (stream)->fd)

#define EVA_STREAM_FD_USE_GLIB_MAIN_LOOP	0

#if !EVA_STREAM_FD_USE_GLIB_MAIN_LOOP
#include "evamainloop.h"
#endif

/* --- structures --- */
struct _EvaStreamFdClass 
{
  EvaStreamClass stream_class;
};
struct _EvaStreamFd 
{
  EvaStream      stream;

  /* read-only */
  guint is_pollable : 1;
  guint is_shutdownable : 1;
  guint is_resolving_name : 1;
  guint failed_name_resolution : 1;

  int fd;
  gushort post_connecting_events;
#if EVA_STREAM_FD_USE_GLIB_MAIN_LOOP
  GPollFD poll_fd;
  GSource *source;
#else
  EvaSource *source;
#endif
};

/* --- prototypes --- */
typedef enum
{
  EVA_STREAM_FD_IS_READABLE     = (1<<0),
  EVA_STREAM_FD_IS_WRITABLE     = (1<<1),
  EVA_STREAM_FD_IS_READWRITE    = EVA_STREAM_FD_IS_READABLE
                                | EVA_STREAM_FD_IS_WRITABLE,
  EVA_STREAM_FD_IS_POLLABLE     = (1<<2),
  EVA_STREAM_FD_IS_SHUTDOWNABLE = (1<<3),
  EVA_STREAM_FD_FOR_NEW_SOCKET  = EVA_STREAM_FD_IS_READWRITE
                                | EVA_STREAM_FD_IS_POLLABLE
			        | EVA_STREAM_FD_IS_SHUTDOWNABLE
} EvaStreamFdFlags;

EvaStream   *eva_stream_fd_new             (gint            fd,
                                            EvaStreamFdFlags flags);
EvaStreamFdFlags eva_stream_fd_flags_guess (gint            fd);
EvaStream   *eva_stream_fd_new_auto        (gint            fd);


EvaStream   *eva_stream_fd_new_connecting  (gint            fd);
EvaStream   *eva_stream_fd_new_from_symbolic_address (EvaSocketAddressSymbolic *symbolic,
                                                      GError                  **error);

/* reading/writing from/to a file */
EvaStream   *eva_stream_fd_new_read_file   (const char     *filename,
					    GError        **error);
EvaStream   *eva_stream_fd_new_write_file  (const char     *filename,
					    gboolean        may_create,
					    gboolean        should_truncate,
					    GError        **error);
EvaStream   *eva_stream_fd_new_create_file (const char     *filename,
					    gboolean        may_exist,
					    GError        **error);


/*< private >*/
EvaStream * eva_stream_fd_new_open (const char     *filename,
			            guint           open_flags,
			            guint           permission,
			            GError        **error);

gboolean    eva_stream_fd_pipe     (EvaStream     **read_side_out,
                                    EvaStream     **write_side_out,
			            GError        **error);

gboolean    eva_stream_fd_duplex_pipe (EvaStream     **side_a_out,
                                       EvaStream     **side_b_out,
			               GError        **error);
gboolean    eva_stream_fd_duplex_pipe_fd (EvaStream     **side_a_out,
                                          int            *side_b_fd_out,
			                  GError        **error);

G_END_DECLS

#endif
