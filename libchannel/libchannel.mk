OBJ = ch.o

CFLAGS = -O3 -fPIC --std=c99 -Wall -Wextra -Wno-unused -I . -I ../libfh -Wcomment -pthread
# CFLAGS = -g -O0 -fPIC --std=c99 -Wall -Wextra -Wno-unused -I . -Wcomment  -I ../libfh -I../libll -Wcomment -pthread

lib: $(OBJ)

#	$(CC) -fPIC -Wl,-undefined -Wl,dynamic_lookup -shared -o libch.so $(OBJ)
	ar csr libch.a $(OBJ)

macosx-lib: $(OBJ)
	$(CC) -fPIC -Wl,-undefined -Wl,dynamic_lookup -shared -o libch.so $(OBJ)
	$(CXX) -dynamiclib -undefined suppress -flat_namespace $(OBJ) -o libch.dylib

clean:
	rm -rf $(OBJ) libch.so* libch.a

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
ch.o: ch.h
