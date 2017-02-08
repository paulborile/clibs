# chash
C Hastables

libfh : fast hash, basic multi threading support, key is only string, opaque data can be string (datalen = -1), fixed lenght (datalen = sizeof data) or datalen = 0 just copies void pointer. Sample code :

```
#include <stdio.h>
#include <pthread.h>
#include "fh.h"

int main(int argc, char **argv)
{
    fh_t *f = fh_create(1000, -1, NULL); // opaque data is string, no hash function

    int err = fh_insert(f, "thekey", "value");

    char str[64];
    err = fh_search(f, "thekey", str);

    printf("value for the key : %s\n", str);

    fh_del(f, "thekey");

    fh_destroy(f);
}

```

libmtfh : multi thread optimized fast hash, still to be uploaded
