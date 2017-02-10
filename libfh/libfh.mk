OBJ = fh.o

CFLAGS = -O3 -fPIC --std=c99 -Wall -Wextra -Wno-unused -I . -Wcomment
# CFLAGS = -g -O0 -fPIC --std=c99 -Wall -Wextra -Wno-unused -I . -Wcomment

lib: $(OBJ)

#	$(CC) -fPIC -Wl,-undefined -Wl,dynamic_lookup -shared -o libfh.so $(OBJ) 
	ar csr libfh.a $(OBJ)

macosx-lib: $(OBJ)
	$(CC) -fPIC -Wl,-undefined -Wl,dynamic_lookup -shared -o libfh.so $(OBJ)
	$(CXX) -dynamiclib -undefined suppress -flat_namespace $(OBJ) -o libufa.dylib

clean:
	rm -rf $(OBJ) libfh.so* libfh.a

#some Windows / MINGW stuff
win-lib: $(OBJ)
	$(CC) -o fh.dll $(OBJ) -shared -Wl,--subsystem,windows,--out-implib,fh.lib

win-clean:

	rm -rf *.o
	rm -rf *.a
	rm -rf *.lib
	rm -rf *.dll
	rm -rf .\test\*.o
	rm -rf *.so
	rm -rf *.exe

