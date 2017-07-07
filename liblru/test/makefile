LRUTESTOBJ = test.o ../../libfh/test/timing.o

CFLAGS=-I .. -I ../../libfh -I ../../libfh/test -pthread -g

all:
	make lrutest
	make example
	make ua-cache
	make lrusim

lrutest: $(LRUTESTOBJ)
	cd ../../libfh ; make -f libfh.mk clean lib
	gcc -o $@ $(LRUTESTOBJ) -L .. -L ../../libfh -llru -lfh -lpthread

example : example.o
	gcc -o $@ $@.o -L .. -L ../../libfh -llru -lfh -lpthread

clean:
	rm -rf *.o lrutest lrusim ua-cache example

ua-cache:
	gcc -g -o ua-cache ua-cache.c ../../libfh/test/timing.o -lwurfl -llru -I .. -L .. -I ../../libfh -I ../../libfh/test -L ../../libfh -lfh

lrusim:
	gcc -g -o lrusim lrusim.c -llru -I .. -L .. -I ../../libfh -I ../../libfh/test -L ../../libfh -lfh
