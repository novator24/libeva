CC ?= gcc
PREFIX ?= /usr/local
CFLAGS = -g -fPIC -Isrc
LDFLAGS = -Wl,-rpath -Wl,.
OBJS = \
        evahello.o                 
INCS = \
        src/eva/evahello.h               

all: libeva.a libeva.so

clean:
	-rm -f *.o *.so *.a libeva.so.* libeva.a

install: libeva.a libeva.so
	mkdir -p ${PREFIX}/include
	mkdir -p ${PREFIX}/lib
	rm -f ${PREFIX}/lib/libeva.a
	rm -f ${PREFIX}/lib/libeva.so
	cp libeva.a ${PREFIX}/lib/libeva-0.1-1.a
	ln -s ${PREFIX}/lib/libeva-0.1-1.a ${PREFIX}/lib/libeva.a
	cp libeva.so ${PREFIX}/lib/libeva-0.1-1.so
	ln -s ${PREFIX}/lib/libeva-0.1-1.so ${PREFIX}/lib/libeva.so
	cp ${INCS} ${PREFIX}/include

libeva.a: $(OBJS)
	ar cr $@ $^

libeva.so: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -Wl,-soname=$@ -Wl,-h -Wl,$@ -shared -L. -lpthread -ldl -lstdc++

evahello.o: src/eva/evahello.c src/eva/evahello.h
	$(CC) -c $(CFLAGS) -o $@ src/eva/evahello.c

