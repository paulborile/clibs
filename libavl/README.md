# libavl

libavl : experimental AVL tree library, still not thread safe, only [char *][void *] key, payload.

## methods

```
avl_h *avl_create(avl_h *static_ah);
int avl_insert(avl_h *ah, const char *key, void *data);
void *avl_search(avl_h *ah, const char *key);
void *avl_del(avl_h *ah, const char *key);
void avl_destroy(avl_h *ah);

```

## build

```
cd libavl
make -f libavl.mk releaseclean releaselinux debugclean debuglinux
Parsing: avl.c as language C
Parsing: avl.h as language C
Parsing: avl_internal.c as language C
rm -rf avl.o avl_internal.o libavl.a
cc -DNDEBUG -O3 -fPIC --std=gnu99 -Wall -Wextra -Wcomment -pthread -I .    -c -o avl.o avl.c
cc -DNDEBUG -O3 -fPIC --std=gnu99 -Wall -Wextra -Wcomment -pthread -I .    -c -o avl_internal.o avl_internal.c
ar crs libavl.a avl.o avl_internal.o
rm -rf avl.do avl_internal.do avl.mo avl_internal.mo libavl-debug.a
cc -DDEBUG -g3 -fsanitize=address -O0 -fPIC --std=gnu99 -Wall -Wextra -Wcomment -pthread -I .  -c avl.c -o avl.do
cc -DDEBUG -g3 -fsanitize=address -O0 -fPIC --std=gnu99 -Wall -Wextra -Wcomment -pthread -I .  -c avl_internal.c -o avl_internal.do
ar crs libavl-debug.a avl.do avl_internal.do
```

To run the tests and benchmarks (in debug mode) : 

```
cd libavl/test
make -f libavl-test.mk testclean testlinux testrun
```

To run only the benchmarks (in release mode)

```
cd libavl/test
make -f libavl-test.mk rtestclean rtestlinux
./rtestavl --gtest_filter=notest
```
