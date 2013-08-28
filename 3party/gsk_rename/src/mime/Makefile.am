noinst_LTLIBRARIES = libeva-mime.la
pkginclude_mimedir = $(includedir)/eva-1.0/eva/mime

INCLUDES = @GLIB_CFLAGS@ @EVA_DEBUG_CFLAGS@

libeva_mime_la_SOURCES = \
evamimeencodings.c \
evamimeencodings-base64.c \
evamimeencodings-identity.c \
evamimeencodings-quotedprintable.c \
evamimemultipartdecoder.c \
evamimemultipartencoder.c \
evamimemultipartpiece.c

pkginclude_mime_HEADERS = \
evamimeencodings.h \
evamimemultipartdecoder.h \
evamimemultipartencoder.h \
evamimemultipartpiece.h
