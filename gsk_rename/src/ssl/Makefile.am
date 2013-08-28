noinst_LTLIBRARIES = libeva-ssl.la
pkginclude_ssldir = $(includedir)/eva-1.0/eva/ssl

INCLUDES = @GLIB_CFLAGS@ @EVA_DEBUG_CFLAGS@ @OPENSSL_CFLAGS@

libeva_ssl_la_LIBADD = @OPENSSL_LIBS@

libeva_ssl_la_SOURCES = \
evaopensslbiostream.c \
evastreamssl.c \
evastreamlistenerssl.c

pkginclude_ssl_HEADERS = \
evaopensslbiostream.h \
evastreamssl.h \
evastreamlistenerssl.h
