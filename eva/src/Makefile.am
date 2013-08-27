if HAVE_OPENSSL
ssl_dirs = ssl
ssl_libs = ssl/libeva-ssl.la
else
ssl_dirs = 
ssl_libs =
endif

composite_headers = \
eva.h \
evahttp.h \
evazlib.h \
evadns.h

SUBDIRS = main-loops common dns zlib $(ssl_dirs) hash mime http xmlrpc url store control . programs tests
INCLUDES = @GLIB_CFLAGS@ @EVA_DEBUG_CFLAGS@
evaincludedir = $(includedir)/eva-1.0/eva
evainclude_HEADERS = \
$(composite_headers) \
evabuffer.h \
evabufferstream.h \
evadaemonize.h \
evadebug.h \
evadebugalloc.h \
evamainloop.h \
evamemorybarrier.h \
evaerrno.h \
evaerror.h \
evafork.h \
evaghelpers.h \
evahook.h \
evainit.h \
evaio.h \
evaipv4.h \
evalistmacros.h \
evalog.h \
evalogringbuffer.h \
evamacros.h \
evamain.h \
evamemory.h \
evamempool.h \
evamodule.h \
evanameresolver.h \
evanetworkinterface.h \
evapacket.h \
evapacketqueue.h \
evapacketqueuefd.h \
evapassfd.h \
evapersistentconnection.h \
evaprocessinfo.h \
evaqsortmacro.h \
evarequest.h \
evarbtreemacros.h \
evasocketaddress.h \
evasocketaddresssymbolic.h \
evasimplefilter.h \
evastdio.h \
evastreamqueue.h \
evastreamconcat.h \
evastream.h \
evastreamclient.h \
evastreamconnection.h \
evastreamexternal.h \
evastreamfd.h \
evastreamlistener.h \
evastreamlistenersocket.h \
evastreamtransferrequest.h \
evastreamwatchdog.h \
evastringmatcher.h \
evatable.h \
evathreadpool.h \
evatree.h \
evatypes.h \
evautils.h

lib_LTLIBRARIES = libeva-1.0.la
libeva_1_0_la_SOURCES = \
inlines.c \
evabuffer.c \
evabufferstream.c \
evadaemonize.c \
evadebug.c \
evadebugalloc.c \
evaerrno.c \
evaerror.c \
evafork.c \
evaghelpers.c \
evahook.c \
evainit.c \
evainlines.c \
evaio.c \
evaipv4.c \
evalog.c \
evalogringbuffer.c \
evamain.c \
evamainloop.c \
evamemory.c \
evamempool.c \
evamodule.c \
evanameresolver.c \
evanetworkinterface.c \
evapacket.c \
evapacketqueue.c \
evapacketqueuefd.c \
evapassfd.c \
evapersistentconnection.c \
evaprocessinfo.c \
evarequest.c \
evasimplefilter.c \
evasocketaddress.c \
evasocketaddresssymbolic.c \
evasocketaddress-connect.c \
evastdio.c \
evastream.c \
evastreamqueue.c \
evastreamconcat.c \
evastreamconnection.c \
evastreamclient.c \
evastreamexternal.c \
evastreamfd.c \
evastreamlistener.c \
evastreamlistenersocket.c \
evastreamtransferrequest.c \
evastreamwatchdog.c \
evatable-flat.c \
evatable-options.c \
evatable.c \
evathreadpool.c \
evatree.c \
evatypes.c \
evautils.c

noinst_HEADERS = \
evatable-file.h \
evatable-helpers.h \
evatable-implement-run-merge-task.inc.c \
debug.h \
cycle.h

evaerror.lo: evaerror.inc

libeva_1_0_la_LDFLAGS = @GLIB_LIBS@
libeva_1_0_la_LIBADD = \
main-loops/libeva-mainloops.la \
dns/libeva-dns.la \
common/libeva-common.la \
http/libeva-http.la \
mime/libeva-mime.la \
$(ssl_libs) \
url/libeva-url.la \
xmlrpc/libeva-xmlrpc.la \
zlib/libeva-zlib.la \
hash/libeva-hash.la \
control/libeva-control.la \
store/libeva-store.la

BUILT_SOURCES = evamemorybarrier.h evatable-implementations-generated.c

evatable-implementations-generated.c: $(srcdir)/gen-eva-table-includes.pl
	perl $(srcdir)/gen-eva-table-includes.pl > evatable-implementations-generated.c

EXTRA_DIST = gen-eva-table-includes.pl

evaerror.inc: $(srcdir)/evaerror.h
	glib-mkenums  --vprod '{@VALUENAME@, "@VALUENAME@", "@valuenick@"},' $(srcdir)/evaerror.h > evaerror.inc

# HACK
missing-refs:
	$(MAKE) all 2>&1 | grep 'undefined reference' | sed -e 's/.*`//;' -e "s/'.*//" | sort -u
