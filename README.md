# clibs - collections in C

This repo contains standard components used to develop faster in C. All collection try to apply the same concepts :
- multi-instantiable : a create function allocates-returns/sets-a-preallocated handle type to be used to refer to that instance of the object
- information hiding : user data is threated as a payload and payloads may of type STRING, VOIDP or FIXEDLEN (*_DATALEN_STRING, *_DATALEN_VOIDP or size)
- thread safeness : when this applies the collection methods (excluding create/destroy) can be used safely by different threads 

Available components :

- fh : a fast, multi-thread optimized open hash hashtable
- channel : a golang inspired channel object with multi-thread aware blocking get operations
- vector : a simple dynamic vector
- lru : lru cache based on fh


## C Hashtable

libfh : fast hashtable, advanced multi threading support, key is only string and is always copied inside (unless FH_SETATTR_DONTCOPYKEY is set),
opaque data is allocated and copied inside hash and can be string (datalen = FH_DATALEN_STRING), fixed lenght (datalen = sizeof data) or datalen = FH_DATALEN_VOIDP just copies void pointer.
Sample code :

```
#include <stdio.h>
#include <pthread.h>
#include "fh.h"

int main(int argc, char **argv)
{
    fh_t *f = fh_create(1000, FH_DATALEN_STRING, NULL); // opaque data is string, using builtin hash function

    int err = fh_insert(f, "thekey", "value");

    char str[64];
    err = fh_search(f, "thekey", str, 64); // search will copy out data

    printf("value for the key : %s\n", str);

	// to get only the pointer to data
	char *value = fh_get(f, "thekey", &err);

    printf("value for the key : %s\n", value);

    fh_del(f, "thekey");

    fh_destroy(f);
}

```
To compile :

```
cd libfh ; make -f libfh.mk releaseclean releaselinux debugclean debuglinux
cd test
make -f libfh-test.mk testclean testlinux testrun
```

Performance : run on Intel Core i7-4710HQ CPU @ 2.50GHz

```
------------ Testing fixed size opaque data hashtable
hash real size 65536
Average insert time in nanosecs : 230.40
hash elements 30000
hash collision 6023
searching ..
Average access time in nanosecs : 154.05
deleting ..
hash elements 0
------------ Testing string opaque data
hash real size 65536
Average insert time in nanosecs : 155.98
hash elements 30000
hash collision 6005
searching ..
Average access time in nanosecs : 111.69
deleting ..
hash elements 0
------------ Testing void pointer opaque data
hash real size 65536
Average insert time in nanosecs : 147.02
hash elements 30000
hash collision 6005
searching ..
Average access time in nanosecs : 81.54
deleting ..
hash elements 0
------------ end of tests
```

## C LRU cache

liblru : is a fast, thread safe Least Recently Used cache, basically a fixed size hashtable that discards least used items first. It is based on libfh (Fast Hash, se below) and on an internal lru list that keeps items ordered by use. Keys are always strings and payloads are void * so you will have to allocate everything outside and take care of freeing as well when/if needed.
Sample code :

```

#include <stdio.h>
#include <stdlib.h>
#include "lru.h"

int main(int argc, char **argv)
{
    char *str;
    // create the lru
    lru_t *l = lru_create(1000);

    // check if an entry exists

    if ( lru_check(l, "this is the key", (void *) &str) != LRU_OK)
    {
        // not found, add it
        lru_add(l, "this is the key", "key payload");
    }

    // destroy

    lru_destroy(l);

    exit(1);
}

```

To compile the liblru and test it :

```
cd liblru ; make -f liblru.mk clean lib
cd test ; make ; ./lrutest <lrusize>

```

Performance : run on Intel Core i7-4710HQ CPU @ 2.50GHz :

```
$ ./lrutest 100000
Average lru_check time in nanosecs : 295.28
Average lru_add time in nanosecs : 156.32

```

