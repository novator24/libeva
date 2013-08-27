INCLUDES = @GLIB_CFLAGS@ @EVA_DEBUG_CFLAGS@
noinst_LTLIBRARIES = libeva-mainloops.la

libeva_mainloops_la_SOURCES = \
evamainloopepoll.c \
evamainloopepoll.h \
evamainloopdevpoll.c \
evamainloopdevpoll.h \
evamainloopkqueue.c \
evamainloopkqueue.h \
evamainlooppoll.c \
evamainlooppoll.h \
evamainlooppollbase.c \
evamainlooppollbase.h \
evamainloopselect.c \
evamainloopselect.h

libeva_mainloops_la_LIBADD = @EPOLL_LIBS@
