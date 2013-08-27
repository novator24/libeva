noinst_LTLIBRARIES = libeva-http.la
pkginclude_httpdir = $(includedir)/eva-1.0/eva/http

INCLUDES = @GLIB_CFLAGS@ @EVA_DEBUG_CFLAGS@

libeva_http_la_SOURCES = \
evahttpclient.c \
evahttpcontent.c \
evahttpheader.c \
evahttpheader-input.c \
evahttpheader-output.c \
evahttprequest.c \
evahttpresponse.c \
evahttpserver.c \
evaprefixtree.c

noinst_HEADERS = evaprefixtree.h

pkginclude_http_HEADERS = \
evahttpclient.h \
evahttpcontent.h \
evahttpheader.h \
evahttprequest.h \
evahttpresponse.h \
evahttpserver.h

evahttpheader.lo: evahttpheader.inc

evahttpheader.inc: $(srcdir)/evahttpheader.h 
	$(GLIB_MKENUMS) --vhead 'static const GEnumValue @enum_name@_enum_values[] = {' \
	                --vprod '{@VALUENAME@, "@VALUENAME@", "@valuenick@"},' \
			--vtail '{0,NULL,NULL}};' $(srcdir)/evahttpheader.h > evahttpheader.inc
