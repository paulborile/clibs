OBJ = lru.o ll.o

CFLAGS = -O3 -fPIC --std=c99 -Wall -Wextra -Wno-unused -I . -I ../libfh -Wcomment -pthread
# CFLAGS = -g -O0 -fPIC --std=c99 -Wall -Wextra -Wno-unused -I . -Wcomment  -I ../libfh -I../libll -Wcomment -pthread

lib: $(OBJ)

#	$(CC) -fPIC -Wl,-undefined -Wl,dynamic_lookup -shared -o liblru.so $(OBJ)
	ar csr liblru.a $(OBJ)

macosx-lib: $(OBJ)
	$(CC) -fPIC -Wl,-undefined -Wl,dynamic_lookup -shared -o liblru.so $(OBJ)
	$(CXX) -dynamiclib -undefined suppress -flat_namespace $(OBJ) -o liblru.dylib

clean:
	rm -rf $(OBJ) liblru.so* liblru.a

#some Windows / MINGW stuff
win-lib: $(OBJ)
	$(CC) -o lru.dll $(OBJ) -shared -Wl,--subsystem,windows,--out-implib,lru.lib

win-clean:

	rm -rf *.o
	rm -rf *.a
	rm -rf *.lib
	rm -rf *.dll
	rm -rf .\test\*.o
	rm -rf *.so
	rm -rf *.exe
