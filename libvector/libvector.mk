OBJ = v.o

CFLAGS = -O3 -fPIC --std=c99 -Wall -Wextra -Wno-unused -I . -Wcomment -pthread
# CFLAGS = -g -O0 -fPIC --std=c99 -Wall -Wextra -Wno-unused -I . -Wcomment  -Wcomment -pthread

lib: $(OBJ)

#	$(CC) -fPIC -Wl,-undefined -Wl,dynamic_lookup -shared -o libv.so $(OBJ)
	ar csr libv.a $(OBJ)

macosx-lib: $(OBJ)
	$(CC) -fPIC -Wl,-undefined -Wl,dynamic_lookup -shared -o libv.so $(OBJ)
	$(CXX) -dynamiclib -undefined suppress -flat_namespace $(OBJ) -o libv.dylib

clean:
	rm -rf $(OBJ) libv.so* libv.a

#some Windows / MINGW stuff
win-lib: $(OBJ)
	$(CC) -o ch.dll $(OBJ) -shared -Wl,--subsystem,windows,--out-implib,ch.lib

win-clean:

	rm -rf *.o
	rm -rf *.a
	rm -rf *.lib
	rm -rf *.dll
	rm -rf .\test\*.o
	rm -rf *.so
	rm -rf *.exe

# depends
v.o: v.h
