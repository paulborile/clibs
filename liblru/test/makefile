LRUTESTOBJ = test.o ../../libfh/test/timing.o

CFLAGS=-I .. -I ../../libfh -I ../../libfh/test -I ../../libll

lrutest: $(LRUTESTOBJ)
	cd ../../libfh ; make -f libfh.mk clean lib
	cd ../../libll ; make -f libll.mk clean lib
	gcc -o $@ $(LRUTESTOBJ) -L .. -L ../../libfh -L ../../libll -llru -lfh -lll

example : example.o
	gcc -o $@ $@.o -L .. -L ../../libfh -L ../../libll -llru -lfh -lll

clean:
	rm -rf *.o lrutest

