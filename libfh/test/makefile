OBJ = test.o
LOCAL_CFLAGS = 

CFLAGS = -g -I.. 

fhtest: $(OBJ)
	$(CC) -o $@ $(OBJ) -lfh -L ..

clean:
	rm -rf $(OBJ) fhtest
