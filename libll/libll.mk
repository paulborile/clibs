OBJ = ll.o

# CFLAGS = -O3 -fPIC --std=c99 -Wall -Wextra -Wno-unused -I . -Wcomment -pthread
CFLAGS = -g -O0 -fPIC --std=c99 -Wall -Wextra -Wno-unused -I . -Wcomment -pthread

lib: $(OBJ)

#	$(CC) -fPIC -Wl,-undefined -Wl,dynamic_lookup -shared -o libll.so $(OBJ)
	ar csr libll.a $(OBJ)

macosx-lib: $(OBJ)
	$(CC) -fPIC -Wl,-undefined -Wl,dynamic_lookup -shared -o libll.so $(OBJ)
	$(CXX) -dynamiclib -undefined suppress -flat_namespace $(OBJ) -o libll.dylib

clean:
	rm -rf $(OBJ) libll.so* liblru.a

#some Windows / MINGW stuff
win-lib: $(OBJ)
	$(CC) -o ll.dll $(OBJ) -shared -Wl,--subsystem,windows,--out-implib,ll.lib

win-clean:

	rm -rf *.o
	rm -rf *.a
	rm -rf *.lib
	rm -rf *.dll
	rm -rf .\test\*.o
	rm -rf *.so
	rm -rf *.exe
