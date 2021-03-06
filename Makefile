CC ?= gcc
PREFIX ?= /usr/local
GLIB_DIR ?= /usr
CFLAGS = -g -std=c99 -fPIC -I$(GLIB_DIR)/lib/glib-2.0/include -I$(GLIB_DIR)/include/glib-2.0 -Isrc
LDFLAGS = -Wl,-rpath,$(GLIB_DIR)/lib -Wl,-rpath,. -lglib-2.0 -L$(GLIB_DIR)/lib
OBJS =  evahello.o \
	evathreadpool.o \
	evainit.o \
	evamainloop.o \
	evaerrno.o \
	evaghelpers.o \
	evaerror.o \
	evabuffer.o \
	evanameresolver.o \
	evadebug.o \
	evasocketaddress.o \
	evaipv4.o \
	evapacketqueuefd.o \
	evapacketqueue.o \
	evapacket.o \
	evaio.o \
	evahook.o \
	evautils.o \
	evatypes.o \
	evafork.o \
	evatree.o
#	evamacros.o 
#	evarbtreemacros.o
#	config.o 
#	cycle.o
#	debug.o

INCS =  src/evask/evahello.h \
	src/evask/evathreadpool.h \
	src/evask/evainit.h \
	src/evask/evamainloop.h \
	src/evask/evaerrno.h \
	src/evask/evaghelpers.h \
	src/evask/evaerror.h src/evask/evaerror.inc \
	src/evask/evabuffer.h \
	src/evask/evanameresolver.h \
	src/evask/evadebug.h \
	src/evask/evasocketaddress.h \
	src/evask/evaipv4.h \
 	src/evask/evapacketqueuefd.h \
 	src/evask/evapacketqueue.h \
 	src/evask/evapacket.h \
 	src/evask/evaio.h \
 	src/evask/evahook.h \
 	src/evask/evautils.h \
 	src/evask/evatypes.h \
 	src/evask/evafork.h \
 	src/evask/evatree.h \
	src/evask/evamacros.h \
	src/evask/evarbtreemacros.h \
	src/evask/config.h \
	src/evask/cycle.h \
	src/evask/debug.h

DNSOBJS = dns_evadnsclient.o \
	dns_evadns.o \
	dns_evadnsrrcache.o \
	dns_evadnsresolver.o 

DNSINCS = src/evask/dns/evadnsclient.h \
	src/evask/dns/evadns.h \
	src/evask/dns/evadnsrrcache.h \
	src/evask/dns/evadnsresolver.h

all: libeva.a libeva.so

clean:
	-rm -f *.o *.so *.a libeva.so.* libeva.a

install: libeva.a libeva.so
	mkdir -p ${PREFIX}/include/eva
	mkdir -p ${PREFIX}/include/eva/dns
	mkdir -p ${PREFIX}/lib
	rm -f ${PREFIX}/lib/libeva.a
	rm -f ${PREFIX}/lib/libeva.so
	cp libeva.a ${PREFIX}/lib/libeva-0.1-1.a
	ln -s ${PREFIX}/lib/libeva-0.1-1.a ${PREFIX}/lib/libeva.a
	cp libeva.so ${PREFIX}/lib/libeva-0.1-1.so
	ln -s ${PREFIX}/lib/libeva-0.1-1.so ${PREFIX}/lib/libeva.so
	cp ${INCS} ${PREFIX}/include/eva
	cp ${DNSINCS} ${PREFIX}/include/eva/dns

libeva.a: $(OBJS) $(DNSOBJS)
	ar cr $@ $^

libeva.so: $(OBJS) $(DNSOBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -Wl,-soname=$@ -Wl,-h -Wl,$@ -shared -L. -lpthread -ldl #-lstdc++

evahello.o: src/evask/evahello.c src/evask/evahello.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evahello.c

evathreadpool.o: src/evask/evathreadpool.c src/evask/evathreadpool.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evathreadpool.c

evainit.o: src/evask/evainit.c src/evask/evainit.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evainit.c

evamainloop.o: src/evask/evamainloop.c src/evask/evamainloop.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evamainloop.c

evaerrno.o: src/evask/evaerrno.c src/evask/evaerrno.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evaerrno.c

evaghelpers.o: src/evask/evaghelpers.c src/evask/evaghelpers.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evaghelpers.c

evaerror.o: src/evask/evaerror.c src/evask/evaerror.h src/evask/evaerror.inc
	$(CC) -c $(CFLAGS) -o $@ src/evask/evaerror.c

evabuffer.o: src/evask/evabuffer.c src/evask/evabuffer.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evabuffer.c

evanameresolver.o: src/evask/evanameresolver.c src/evask/evanameresolver.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evanameresolver.c

evadebug.o: src/evask/evadebug.c src/evask/evadebug.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evadebug.c

evasocketaddress.o: src/evask/evasocketaddress.c src/evask/evasocketaddress.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evasocketaddress.c

evaipv4.o: src/evask/evaipv4.c src/evask/evaipv4.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evaipv4.c

evapacketqueuefd.o: src/evask/evapacketqueuefd.c src/evask/evapacketqueuefd.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evapacketqueuefd.c

evapacketqueue.o: src/evask/evapacketqueue.c src/evask/evapacketqueue.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evapacketqueue.c

evapacket.o: src/evask/evapacket.c src/evask/evapacket.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evapacket.c

evaio.o: src/evask/evaio.c src/evask/evaio.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evaio.c

evahook.o: src/evask/evahook.c src/evask/evahook.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evahook.c

evautils.o: src/evask/evautils.c src/evask/evautils.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evautils.c

evatypes.o: src/evask/evatypes.c src/evask/evatypes.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evatypes.c

evafork.o: src/evask/evafork.c src/evask/evafork.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evafork.c

evatree.o: src/evask/evatree.c src/evask/evatree.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/evatree.c

#evamacros.o: src/evask/evamacros.c src/evask/evamacros.h
#	$(CC) -c $(CFLAGS) -o $@ src/evask/evamacros.c

#evarbtreemacros.o: src/evask/evarbtreemacros.c src/evask/evarbtreemacros.h
#	$(CC) -c $(CFLAGS) -o $@ src/evask/evarbtreemacros.c

#config.o: src/evask/config.c src/evask/config.h
#	$(CC) -c $(CFLAGS) -o $@ src/evask/config.c

#cycle.o: src/evask/cycle.c src/evask/cycle.h
#	$(CC) -c $(CFLAGS) -o $@ src/evask/cycle.c

#debug.o: src/evask/debug.c src/evask/debug.h
#	$(CC) -c $(CFLAGS) -o $@ src/evask/debug.c

dns_evadnsclient.o: src/evask/dns/evadnsclient.c src/evask/dns/evadnsclient.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/dns/evadnsclient.c

dns_evadns.o: src/evask/dns/evadns.c src/evask/dns/evadns.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/dns/evadns.c

dns_evadnsrrcache.o: src/evask/dns/evadnsrrcache.c src/evask/dns/evadnsrrcache.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/dns/evadnsrrcache.c

dns_evadnsresolver.o: src/evask/dns/evadnsresolver.c src/evask/dns/evadnsresolver.h
	$(CC) -c $(CFLAGS) -o $@ src/evask/dns/evadnsresolver.c

