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

