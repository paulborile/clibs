OBJ = test.o timing.o
LOCAL_CFLAGS =

CFLAGS = -g -I../..

fhtest: $(OBJ)
	$(CC) -o $@ $(OBJ) -lfh -L ../.. -lpthread

clean:
	rm -rf $(OBJ) fhtest
