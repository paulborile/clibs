# clibs - collections in C

This repo contains standard components used to develop faster in C. All libraries try to apply the same concepts :
- multi-instantiable : a create function allocates-returns/sets-a-preallocated handle type to be used to refer to that instance of the object
- information hiding : user data is threated as a payload and payloads may of type STRING, VOIDP or FIXEDLEN (*_DATALEN_STRING, *_DATALEN_VOIDP or size)
- thread safeness : when this applies the collection methods (excluding create/destroy) can be used safely by different threads 

Available components :

- fh : a fast, multi-thread optimized open hash hashtable
- channel : a golang inspired channel object with multi-thread aware blocking get operations
- vector : a simple dynamic vector
- lru : lru cache based on fh


## C Hashtable

libfh : fast hashtable, advanced multi threading support, key is only string and is always copied inside 
(unless FH_SETATTR_DONTCOPYKEY is set), opaque data is allocated and copied inside hash and can be string 
(datalen = FH_DATALEN_STRING), fixed lenght (datalen = sizeof data) or datalen = FH_DATALEN_VOIDP just 
copies void pointer.
hashfunction is oat hash (one at a time hash) by Bob Jenkins but you can set your hash function in fh_create().
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

## C Thread pool

libthp : simple thread pool library in C. Easy to use :

```
	...
	// create thread pool with 10 running threads
    thp_h *t = thp_create(NULL, 10, &err);

	// add some work to thread
	thp_add(t, user_function, (void *) user_function_param);

	// wait that all jobs terminate
	thp_wait(t);

    thp_destroy(t);

```

To compile (you will also need libchannel) :

```
make -f libthp.mk releaseclean releaselinux debugclean debuglinux
cd test
make -f libthp-test.mk testclean testlinux testrun

```

## C Channel (Go inspired) - a FIFO queue with unlimited length with block/non-blocking read

Connect producer / consumer threads with a thread safe data channel

```
    ...
    ch_h *ch = NULL;
    pthread_t th_writer, th_reader;
    int pthread_ret = 0, retval;
    void *th_ret;

    ch_h *ch = ch_create(NULL, CH_DATALEN_STRING);

    // retval = ch_setattr(ch, CH_BLOCKING_MODE, CH_ATTR_NON_BLOCKING_GET);

    pthread_ret = pthread_create(&th_reader, NULL, &thread_reader, (void *)ch);

    for (i = 0; i < MSGS_SENT; i++)
    {
        char text[20];

        sprintf(text, "text %d", i);
        retval = ch_put(ch, text);
    }
    ch_put(ch, CH_ENDOFTRANSMISSION);
    pthread_join(th_reader, &th_ret);
    retval = ch_destroy(ch);
    ...

static void *thread_reader(void *ch)
{
    char text[50];
    int eot = 0;
    while (eot != CH_GET_ENDOFTRANSMISSION)
    {
        eot = ch_get(ch, text);

        if (eot < 0 && eot != CH_GET_ENDOFTRANSMISSION)
        {
            // error
        }
    }
    return NULL;
}

```

```
make -f libch.mk releaseclean releaselinux debugclean debuglinux
cd test
make -f libch-test.mk testclean testlinux testrun

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

