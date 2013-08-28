/* src/config.h.  Generated from config.h.in by configure.  */
/* src/config.h.in.  Generated from configure.in by autoheader.  */

/* whether we GTimeVal is timeval */
#define GTIMEVAL_EQUALS_SYS_TIMEVAL 1

/* whether we have a PF_LOCAL constant */
#define HAS_PF_LOCAL 1

/* whether we have a PF_UNIX constant */
#define HAS_PF_UNIX 1

/* whether the sockaddr structure has a length member */
#define HAS_SOCKADDR_SA_LEN 0

/* Define to 1 if you have the `clock_gettime' function. */
/* #undef HAVE_CLOCK_GETTIME */

/* Define to 1 if you have the <c_asm.h> header file. */
/* #undef HAVE_C_ASM_H */

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* whether to support the new epoll_* functions */
#define HAVE_EPOLL_SUPPORT 1

/* Define to 1 if you have the <execinfo.h> header file. */
#define HAVE_EXECINFO_H 1

/* Define to 1 if you have the `gethrtime' function. */
/* #undef HAVE_GETHRTIME */

/* Define to 1 if you have the `getrusage' function. */
#define HAVE_GETRUSAGE 1

/* Define to 1 if you have the `gmtime_r' function. */
#define HAVE_GMTIME_R 1

/* Define to 1 if hrtime_t is defined in <sys/time.h> */
/* #undef HAVE_HRTIME_T */

/* Define to 1 if you have the <intrinsics.h> header file. */
/* #undef HAVE_INTRINSICS_H */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `kqueue' function. */
/* #undef HAVE_KQUEUE */

/* Define to 1 if you have the `localtime_r' function. */
#define HAVE_LOCALTIME_R 1

/* Define to 1 if you have the `mach_absolute_time' function. */
/* #undef HAVE_MACH_ABSOLUTE_TIME */

/* Define to 1 if you have the <mach/mach_time.h> header file. */
/* #undef HAVE_MACH_MACH_TIME_H */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <net/if.h> header file. */
#define HAVE_NET_IF_H 1

/* Define if you have openssl support. */
#define HAVE_OPENSSL /**/

/* Define to 1 if you have the `poll' function. */
#define HAVE_POLL 1

/* Define to 1 if you have the <readline.h> header file. */
/* #undef HAVE_READLINE_H */

/* Define to 1 if you have the <readline/readline.h> header file. */
/* #undef HAVE_READLINE_READLINE_H */

/* Define to 1 if you have the `read_real_time' function. */
/* #undef HAVE_READ_REAL_TIME */

/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strtoll' function. */
#define HAVE_STRTOLL 1

/* Define to 1 if you have the `strtoq' function. */
#define HAVE_STRTOQ 1

/* Define to 1 if you have the `strtoull' function. */
#define HAVE_STRTOULL 1

/* Define to 1 if you have the `strtouq' function. */
#define HAVE_STRTOUQ 1

/* Define to 1 if you have the `syslog' function. */
#define HAVE_SYSLOG 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/poll.h> header file. */
#define HAVE_SYS_POLL_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the `timegm' function. */
#define HAVE_TIMEGM 1

/* Define to 1 if you have the `time_base_to_time' function. */
/* #undef HAVE_TIME_BASE_TO_TIME */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `writev' function. */
#define HAVE_WRITEV 1

/* Define if you have the UNICOS _rtc() intrinsic. */
/* #undef HAVE__RTC */

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
/* #undef LT_OBJDIR */

/* Name of package */
#define PACKAGE "eva1.0"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* what header to include to get the readline functions */
#define READLINE_HEADER_NAME <>

/* what header to include to get the readline/history functions */
#define READLINE_HISTORY_HEADER_NAME <>

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* ipv6 support */
#define SUPPORTS_IPV6 1

/* 0 or 1 depending on whether the readline library is available */
#define SUPPORT_READLINE 0

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Version number of package */
#define VERSION "1.0.63"

/* Number of bits in a file offset, on hosts where this is settable. */
#define _FILE_OFFSET_BITS 64

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* the type to use for a socket address size */
/* #undef socklen_t */
