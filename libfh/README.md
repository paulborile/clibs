# libfh 

```
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
