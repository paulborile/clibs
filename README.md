# chash
C Hastable

libfh : fast hash, basic multi threading support, key is only string and is always copied inside (TBD attr to not copy), opaque data is always allocated and copied inside hash and can be string (datalen = -1), fixed lenght (datalen = sizeof data) or datalen = 0 just copies void pointer. Sample code :

```
#include <stdio.h>
#include <pthread.h>
#include "fh.h"

int main(int argc, char **argv)
{
    fh_t *f = fh_create(1000, -1, NULL); // opaque data is string, no hash function

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
cd libfh ; make -f libfh.mk clean lib
```


libmtfh : multi thread optimized fast hash, still to be uploaded
