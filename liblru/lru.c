/*
   Copyright (c) 2017, Paul Stephen Borile
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
   notice
   3. Neither the name of the Paul Stephen Borile nor the
   names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY Paul Stephen Borile ''AS IS'' AND ANY
   EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL Paul Stephen Borile BE LIABLE FOR ANY
   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "ll.h"
#include "fh.h"
#include "lru.h"

// fh payload
struct _lru_payload_t {
    void *payload;
    void *ll_slot;
    char *fh_key;
};
typedef struct _lru_payload_t lru_payload_t;

// lru_create : create lru object
// lru is basically an hashtable with a maximum number of elements that can be added. When the lru is full,
// the least recently used entry is removed to make space for the new one.
// It is based on a libfh hastable and on a libll linked list
// key is always a string (char *, '\0' terminated), while datalen is always a void *

lru_t *lru_create(int dim)
{
    lru_t *l = NULL;
    // allocate lru_t
    l = (lru_t *) malloc(sizeof(lru_t));
    if (l == NULL)
    {
        return (NULL);
    }

    l->magic = LRU_MAGIC_ID;
    l->size = dim; // maximum number of objects to kept in hash

    l->fh = fh_create(dim, FH_DATALEN_VOIDP, NULL);
    if ( l->fh == NULL )
    {
        return (NULL);
    }

    l->ll = ll_create(dim);
    if (l->ll == NULL )
    {
        return(NULL);
    }

    return l;
}

// add entry to lru
//

int     lru_add(lru_t *lru, char *key, void *payload)
{
    lru_payload_t *hashpayload;
    int fh_err;
    void *slot;
    lru_payload_t *new;

    // check hastable size

    int hsize = fh_getattr(lru->fh, FH_ATTR_ELEMENT, &hsize);

    if (hsize >= lru->size)
    {
        // lru is full, need to remove an entry, pointer to removed entry payload is returned
        ll_remove_last(lru->ll, (void *)&hashpayload);

        // now remove entry from hashtable by key
        if (( fh_err = fh_del(lru->fh, hashpayload->fh_key)) < 0 )
        {
            // error, fh_del returns error
            printf("fh_del returns %d on %s", fh_err, hashpayload->fh_key);
        }

        // free allocated payload datalen
        free(hashpayload);
    }

    // now allocate a new ll_slot
    slot = ll_slot_new(lru->ll);

    // New fh opaque with lru payload + ll_slot_pointer + fh_key since used for ll too
    new = malloc(sizeof(lru_payload_t));
    assert(new != NULL);

    new->payload = payload;
    new->ll_slot = slot;
    new->fh_key = key;

    fh_insert(lru->fh, key, new);

    ll_slot_set_payload(lru->ll, slot, new);

    ll_slot_move_to_top(lru->ll, slot);

    return 1;
}

int     lru_check(lru_t *lru, char *key, void **payload)
{
    lru_payload_t *pload;
    int fherr;

    pload = (lru_payload_t *) fh_get(lru->fh, key, &fherr);

    if ( pload == NULL )
    {
        return LRU_ELEMENT_NOT_FOUND;
    }

    // copy pointer out
    *payload = pload->payload;

    ll_slot_move_to_top(lru->ll, pload->ll_slot);

    return(LRU_OK);
}

int     lru_destroy(lru_t *lru)
{
    void *payload;
    // remove all allocated payloads
    while ( ll_remove_last(lru->ll, &payload) != NULL )
    {
        free(payload);
    }

    ll_destroy(lru->ll);
    fh_destroy(lru->fh);
    free(lru);

    return(1);
}


#ifdef TEST

int main(int argc, char **argv)
{
    int howmany = atoi(argv[1]);
    char *str = NULL;

    lru_t *l = lru_create(howmany);

    lru_add(l, "unitedstates", "washington");
    lru_add(l, "italy", "rome");
    lru_add(l, "france", "paris");

    lru_check(l, "france", (void *)&str);

    if (strcmp(str, "paris") != 0 )
    {
        printf("error in lru_check, returned %s\n", str);
    }
}

#endif
