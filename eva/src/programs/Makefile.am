INCLUDES = @GLIB_CFLAGS@ @EVA_DEBUG_CFLAGS@

bin_PROGRAMS = eva-control-client eva-netcat eva-wget eva-debug-alloc-tool \
	eva-throttle-proxy eva-webserver eva-escape eva-analyze-successive-memdumps

eva_control_client_SOURCES = eva-control-client-main.c
eva_control_client_LDADD = ../libeva-1.0.la @READLINE_LIBS@ @GLIB_EXTRA_LDFLAGS@

eva_analyze_successive_memdumps_SOURCES = eva-analyze-successive-memdumps-main.c
eva_analyze_successive_memdumps_LDADD = ../libeva-1.0.la @GLIB_EXTRA_LDFLAGS@

eva_netcat_SOURCES = eva-netcat.c
eva_netcat_LDADD = ../libeva-1.0.la @GLIB_EXTRA_LDFLAGS@

eva_wget_SOURCES = eva-wget.c
eva_wget_LDADD = ../libeva-1.0.la @GLIB_EXTRA_LDFLAGS@

eva_webserver_SOURCES = eva-webserver.c
eva_webserver_LDADD = ../libeva-1.0.la @GLIB_EXTRA_LDFLAGS@

eva_throttle_proxy_SOURCES = eva-throttle-proxy.c
eva_throttle_proxy_LDADD = ../libeva-1.0.la @GLIB_EXTRA_LDFLAGS@

eva_debug_alloc_tool_SOURCES = eva-debug-alloc-tool.c evadebuglog.c evadebuglog.h
eva_debug_alloc_tool_LDADD = ../libeva-1.0.la @GLIB_EXTRA_LDFLAGS@

eva_escape_SOURCES = eva-escape.c
eva_escape_LDADD = ../libeva-1.0.la @GLIB_EXTRA_LDFLAGS@

noinst_PROGRAMS = example-server eva-connreset-daemon
example_server_SOURCES = example-server.c
example_server_LDADD = ../libeva-1.0.la @GLIB_EXTRA_LDFLAGS@
eva_connreset_daemon_SOURCES = eva-connreset-daemon.c
eva_connreset_daemon_LDADD = ../libeva-1.0.la @GLIB_EXTRA_LDFLAGS@
