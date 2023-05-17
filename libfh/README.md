# libfh 

libfh : fast hashtable, advanced multi threading support, key is only string and is always copied inside (unless FH_SETATTR_DONTCOPYKEY is set), opaque data is allocated and copied inside hash and can be string (datalen = FH_DATALEN_STRING), fixed lenght (datalen = sizeof data) or datalen = FH_DATALEN_VOIDP just copies void pointer. hashfunction is oat hash (one at a time hash) by Bob Jenkins but you can set your hash function in fh_create()

May 2023 : introduced Murmurhash and benchmarks with picobench ( https://github.com/iboB/picobench )

```
cd libfh
make -f libfh.mk releaseclean releaselinux debugclean debuglinux

Parsing: fh.c as language C
Parsing: fh.h as language C
rm -rf fh.o libfh.a
cc -DNDEBUG -O3 -fPIC --std=c99 -Wall -Wextra -Wcomment -pthread -I .    -c -o fh.o fh.c
ar crs libfh.a fh.o
rm -rf fh.do fh.mo libfh-debug.a
cc -DDEBUG -g3 -fsanitize=address -O0 -fPIC --std=c99 -Wall -Wextra -Wcomment -pthread -I .  -c fh.c -o fh.do
ar crs libfh-debug.a fh.do
```

To run the tests and benchmarks (in debug mode) : 

```
cd libfh/test
make -f libfh-test.mk testclean testlinux testrun
```

To run only the benchmarks (in release mode)

```
cd libfh/test
make -f libfh-test rtestclean rtestlinux
./rtestfh --gtest_filter=notest
```

