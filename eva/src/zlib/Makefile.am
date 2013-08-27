noinst_LTLIBRARIES = libeva-zlib.la
pkginclude_zlibdir = $(includedir)/eva-1.0/eva/zlib

INCLUDES = @GLIB_CFLAGS@ @EVA_DEBUG_CFLAGS@

libeva_zlib_la_LIBADD = -lz

libeva_zlib_la_SOURCES = \
evazlibdeflator.c \
evazlibinflator.c \
evazlib.c 

pkginclude_zlib_HEADERS = \
evazlib.h \
evazlibdeflator.h \
evazlibinflator.h

