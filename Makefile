CC ?= gcc
PREFIX ?= /usr/local
GLIB_DIR ?= /usr
CFLAGS = -g -fPIC -I$(GLIB_DIR)/lib/glib-2.0/include -I$(GLIB_DIR)/include/glib-2.0 -Isrc
LDFLAGS = -Wl,-rpath -Wl,.
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

INCS =  src/eva/evahello.h \
	src/eva/evathreadpool.h \
	src/eva/evainit.h \
	src/eva/evamainloop.h \
	src/eva/evaerrno.h \
	src/eva/evaghelpers.h \
	src/eva/evaerror.h src/eva/evaerror.inc \
	src/eva/evabuffer.h \
	src/eva/evanameresolver.h \
	src/eva/evadebug.h \
	src/eva/evasocketaddress.h \
	src/eva/evaipv4.h \
 	src/eva/evapacketqueuefd.h \
 	src/eva/evapacketqueue.h \
 	src/eva/evapacket.h \
 	src/eva/evaio.h \
 	src/eva/evahook.h \
 	src/eva/evautils.h \
 	src/eva/evatypes.h \
 	src/eva/evafork.h \
 	src/eva/evatree.h \
	src/eva/evamacros.h \
	src/eva/evarbtreemacros.h \
	src/eva/config.h \
	src/eva/cycle.h \
	src/eva/debug.h

DNSOBJS = dns_evadnsclient.o \
	dns_evadns.o \
	dns_evadnsrrcache.o \
	dns_evadnsresolver.o 

DNSINCS = src/eva/dns/evadnsclient.h \
	src/eva/dns/evadns.h \
	src/eva/dns/evadnsrrcache.h \
	src/eva/dns/evadnsresolver.h

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
	$(CC) $(LDFLAGS) -o $@ $^ -Wl,-soname=$@ -Wl,-h -Wl,$@ -shared -L. -lpthread -ldl -lstdc++

evahello.o: src/eva/evahello.c src/eva/evahello.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evahello.c

evathreadpool.o: src/eva/evathreadpool.c src/eva/evathreadpool.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evathreadpool.c

evainit.o: src/eva/evainit.c src/eva/evainit.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evainit.c

evamainloop.o: src/eva/evamainloop.c src/eva/evamainloop.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evamainloop.c

evaerrno.o: src/eva/evaerrno.c src/eva/evaerrno.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evaerrno.c

evaghelpers.o: src/eva/evaghelpers.c src/eva/evaghelpers.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evaghelpers.c

evaerror.o: src/eva/evaerror.c src/eva/evaerror.h src/eva/evaerror.inc
	$(CC) -c $(CFLAGS) -o $@ src/eva/evaerror.c

evabuffer.o: src/eva/evabuffer.c src/eva/evabuffer.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evabuffer.c

evanameresolver.o: src/eva/evanameresolver.c src/eva/evanameresolver.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evanameresolver.c

evadebug.o: src/eva/evadebug.c src/eva/evadebug.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evadebug.c

evasocketaddress.o: src/eva/evasocketaddress.c src/eva/evasocketaddress.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evasocketaddress.c

evaipv4.o: src/eva/evaipv4.c src/eva/evaipv4.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evaipv4.c

evapacketqueuefd.o: src/eva/evapacketqueuefd.c src/eva/evapacketqueuefd.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evapacketqueuefd.c

evapacketqueue.o: src/eva/evapacketqueue.c src/eva/evapacketqueue.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evapacketqueue.c

evapacket.o: src/eva/evapacket.c src/eva/evapacket.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evapacket.c

evaio.o: src/eva/evaio.c src/eva/evaio.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evaio.c

evahook.o: src/eva/evahook.c src/eva/evahook.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evahook.c

evautils.o: src/eva/evautils.c src/eva/evautils.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evautils.c

evatypes.o: src/eva/evatypes.c src/eva/evatypes.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evatypes.c

evafork.o: src/eva/evafork.c src/eva/evafork.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evafork.c

evatree.o: src/eva/evatree.c src/eva/evatree.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evatree.c

#evamacros.o: src/eva/evamacros.c src/eva/evamacros.h
#	$(CC) -c $(CFLAGS) -o $@ src/eva/evamacros.c

#evarbtreemacros.o: src/eva/evarbtreemacros.c src/eva/evarbtreemacros.h
#	$(CC) -c $(CFLAGS) -o $@ src/eva/evarbtreemacros.c

#config.o: src/eva/config.c src/eva/config.h
#	$(CC) -c $(CFLAGS) -o $@ src/eva/config.c

#cycle.o: src/eva/cycle.c src/eva/cycle.h
#	$(CC) -c $(CFLAGS) -o $@ src/eva/cycle.c

#debug.o: src/eva/debug.c src/eva/debug.h
#	$(CC) -c $(CFLAGS) -o $@ src/eva/debug.c

dns_evadnsclient.o: src/eva/dns/evadnsclient.c src/eva/dns/evadnsclient.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/dns/evadnsclient.c

dns_evadns.o: src/eva/dns/evadns.c src/eva/dns/evadns.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/dns/evadns.c

dns_evadnsrrcache.o: src/eva/dns/evadnsrrcache.c src/eva/dns/evadnsrrcache.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/dns/evadnsrrcache.c

dns_evadnsresolver.o: src/eva/dns/evadnsresolver.c src/eva/dns/evadnsresolver.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/dns/evadnsresolver.c

