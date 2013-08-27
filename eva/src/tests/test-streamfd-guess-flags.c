#include "../evastreamfd.h"
#include "../evainit.h"
#include <unistd.h>

#define CHECK_FLAG_TRUE(flags, nick)	\
	g_assert ( ((flags) & EVA_STREAM_FD_##nick) == EVA_STREAM_FD_##nick )
#define CHECK_FLAG_FALSE(flags, nick)	\
	g_assert ( ((flags) & EVA_STREAM_FD_##nick) == 0 )
#define CHECK_FLAG_TRUE_OPT(flags, nick)	\
	G_STMT_START{				\
	    if ( ((flags) & EVA_STREAM_FD_##nick) != EVA_STREAM_FD_##nick ) \
	      { marginal = TRUE; } 		\
	}G_STMT_END
#define CHECK_FLAG_FALSE_OPT(flags, nick)	\
	G_STMT_START {			\
	    if ( ((flags) & EVA_STREAM_FD_##nick) != 0 ) { marginal = TRUE; } \
	}G_STMT_END

int main (int argc, char **argv)
{
  GskStreamFdFlags flags;
  int pipe_fds[2];
  gboolean marginal;
  eva_init_without_threads (&argc, &argv);

  /* Test pipe output. */
  marginal = FALSE;
  g_printerr ("Testing pipe()... ");
  g_assert (pipe (pipe_fds) >= 0);
  flags = eva_stream_fd_flags_guess (pipe_fds[0]);
  CHECK_FLAG_TRUE (flags, IS_READABLE);
  CHECK_FLAG_TRUE (flags, IS_POLLABLE);
  CHECK_FLAG_FALSE_OPT (flags, IS_WRITABLE);
  flags = eva_stream_fd_flags_guess (pipe_fds[1]);
  CHECK_FLAG_FALSE_OPT (flags, IS_READABLE);
  CHECK_FLAG_TRUE (flags, IS_POLLABLE);
  CHECK_FLAG_TRUE (flags, IS_WRITABLE);
  close (pipe_fds[0]);
  close (pipe_fds[1]);
  if (marginal)
    g_printerr ("ok. [thinks pipes are bidirectional]\n");
  else
    g_printerr ("good.\n");

  return 0;
}
