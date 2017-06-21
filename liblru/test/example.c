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
