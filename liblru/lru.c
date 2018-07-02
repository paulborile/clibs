/*
   Copyright (c) 2017, Paul Stephen Borile
   All rights reserved.
   License : MIT

 */

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "ll.h"
#include "lru.h"

// fh payload
struct _lru_payload_t {
    void *payload;
    void *ll_slot;
    char *fh_key;
};
typedef struct _lru_payload_t lru_payload_t;

#define LRU_CHECK(l) if ((!l) || (l->magic != LRU_MAGIC_ID)) return (LRU_BAD_HANDLE)


// lru_create : create lru object
// lru is basically an hashtable with a maximum number of elements that can be added. When the lru is full,
// the least recently used entry is removed to make space for the new one.
// It is based on a libfh hastable and on a libll linked list
// key is always a string (char *, '\0' terminated), while datalen is always a void *

lru_t *lru_create(int dim)
{
    if(dim <= 0)
    {
        return NULL;
    }

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
    // set FH_SETATTR_DONTCOPYKEY because the only copy of the key will be in lru_payload_t
    fh_setattr(l->fh, FH_SETATTR_DONTCOPYKEY, 0);

    l->ll = ll_create(dim, sizeof(lru_payload_t));
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
    void *slot, *slot_to_free;
    lru_payload_t *new;

    LRU_CHECK(lru);

    // ll_slot_new returns NULL on lru full, need to remove an entry and free it and try again
    while (( slot = ll_slot_new(lru->ll, (void *)&new)) == NULL )
    {
        if (( slot_to_free = ll_remove_last(lru->ll, (void *)&hashpayload)) == NULL)
        {
            // freelist is empty (no free slots) but also ll list is empty
            // reproduced with lru size 3 and 4 threads doing add
            return(LRU_NO_MEMORY);
        }

        // now remove entry from hashtable by key
        if (( fh_err = fh_del(lru->fh, hashpayload->fh_key)) < 0 )
        {
            // error, fh_del returns error
            int size;
            fh_getattr(lru->fh, FH_ATTR_ELEMENT, &size);
            printf("fh size = %d, fh_del returns %d on %s, payload %s\n", size, fh_err, hashpayload->fh_key, (char *)hashpayload->payload);
        }

        // free allocated key
        free(hashpayload->fh_key);
        // now that everything has been cleaned put it back to freelist
        ll_slot_free(lru->ll, slot_to_free);
    }

    // slot is a valid new entry now and new is a pointer to the payload (preallocated)
    assert(new);

    new->payload = payload;
    new->ll_slot = slot;

    if ((new->fh_key = malloc(strlen(key) + 1)) == NULL) // this is only copy of key around
    {
        ll_slot_free(lru->ll, slot);
        return(LRU_NO_MEMORY);
    }
    strcpy(new->fh_key, key);

    int lru_err;
    if (( lru_err = fh_insert(lru->fh, new->fh_key, new)) != LRU_OK )
    {
        if (lru_err == FH_DUPLICATED_ELEMENT)
        {
            free(new->fh_key);
            ll_slot_free(lru->ll, slot);
            return LRU_DUPLICATED_ELEMENT;
        }
        if (lru_err == FH_NO_MEMORY)
        {
            free(new->fh_key);
            ll_slot_free(lru->ll, slot);
            return LRU_NO_MEMORY;
        }
    }

    ll_slot_add_to_top(lru->ll, slot);

    return LRU_OK;
}

// check for presence of an element
int     lru_check(lru_t *lru, char *key, void **payload)
{
    lru_payload_t *pload;
    int fhslot;
    int fherr;

    LRU_CHECK(lru);

    pload = (lru_payload_t *) fh_searchlock(lru->fh, key, &fhslot, &fherr);
    // pload = (lru_payload_t *) fh_get(lru->fh, key, &fherr);

    if ( pload == NULL )
    {
        //fh_releaselock(lru->fh, fhslot);
        return LRU_ELEMENT_NOT_FOUND;
    }

    // copy pointer out
    *payload = pload->payload;

    ll_slot_move_to_top(lru->ll, pload->ll_slot);

    fh_releaselock(lru->fh, fhslot);

    return(LRU_OK);
}

// clear the lru, empty it
int   lru_clear(lru_t *lru)
{
    lru_payload_t *hashpayload;
    int fh_err;
    ll_slot_t *removed;

    LRU_CHECK(lru);

    while (( removed = ll_remove_last(lru->ll, (void *)&hashpayload)) != NULL )
    {
        // remove entry from hashtable by key
        if (( fh_err = fh_del(lru->fh, hashpayload->fh_key)) < 0 )
        {
            // error, fh_del returns error
            int size;
            fh_getattr(lru->fh, FH_ATTR_ELEMENT, &size);
            printf("fh size = %d, fh_del returns %d on %s, payload %s\n", size, fh_err, hashpayload->fh_key, (char *)hashpayload->payload);
            return LRU_ELEMENT_NOT_FOUND;
        }

        // free allocated key
        free(hashpayload->fh_key);
        ll_slot_free(lru->ll, removed);
    }

    return LRU_OK;
}

// destroy the object
int     lru_destroy(lru_t *lru)
{
    LRU_CHECK(lru);

    lru_clear(lru);
    ll_destroy(lru->ll);
    fh_destroy(lru->fh);
    free(lru);

    return(1);
}

static int print_payload(void *v)
{
    lru_payload_t *p = v;
    printf("key = %s, payload = %s, ll_slot = %p\n", p->fh_key, (char *)p->payload, p->ll_slot);
    return 0;
}

int lru_print(lru_t *lru)
{
    LRU_CHECK(lru);
    ll_print(lru->ll, print_payload);
    return 0;
}

int lru_get_ll_data(lru_t *lru, int idx, char** key, void** payload, void** ll_slot)
{
    void* voidp = NULL;
    LRU_CHECK(lru);

    int ll_err = ll_get_payload(lru->ll, idx, &voidp);
    if(ll_err != LL_OK)
        return ll_err;

    lru_payload_t *p = voidp;

    (*key) = p->fh_key;
    (*payload) = p->payload;
    (*ll_slot) = p->ll_slot;

    return LRU_OK;
}

int   lru_get_ll_key_position(lru_t *lru, const char* key)
{
    void* voidp = NULL;
    LRU_CHECK(lru);

    int idx = 0;
    while(ll_get_payload(lru->ll, idx, &voidp) == LL_OK)
    {
        lru_payload_t *p = voidp;
        if(strcmp(key, p->fh_key) == 0)
        {
            return idx;
        }

        idx++;
    }

    return -1;
}


#ifdef TEST

#define MAXSIZE 100000
char payloads[MAXSIZE][20];


int main(int argc, char **argv)
{
    int howmany = atoi(argv[1]);
    char *str = NULL;
    int j;

    if (howmany > MAXSIZE)
    {
        printf("Mac size for testing purposes is %d\n", MAXSIZE);
        exit(1);
    }

    for (int i = 0; i < howmany; i++)
    {
        sprintf(payloads[i], "%d", i);
    }

    lru_t *l = lru_create(howmany);

    for (int i = 0; i < howmany*1000; i++)
    {
        j = rand() % howmany;
        if ( lru_check(l, payloads[j], (void *) &str) == LRU_OK)
        {
            // found, test if same
            if ( strcmp(payloads[j], str) != 0 )
            {
                printf("ouch %s != %s\n", payloads[j], str);
                exit(1);
            }
        }
        else
        {
            lru_add(l, payloads[j], (void *) payloads[j]);
        }
    }

    lru_destroy(l);

    // now check that lru is correctly caching members
    // use a lru that is smaller than the payloads set

    l = lru_create(howmany/10);

    // fill with 0->howmany/10
    for (int i = 0; i < howmany/10; i++)
    {
        // j = rand() % howmany;
        j = i;

        lru_add(l, payloads[j], (void *) payloads[j]);
    }

    // check that the next tenth is no present
    for (int i = howmany/10; i < (howmany/10 + howmany/10); i++)
    {
        // j = rand() % howmany;
        j = i;

        if ( lru_check(l, payloads[j], (void *) &str) == LRU_OK)
        {
            // SHOULD NEVER ENTER HERE : lru is full of howmany/10
            printf("this stuff should not be here !! ouch %s != %s\n", payloads[j], str);
            exit(1);
        }
        else
        {
            // adding : it will remove all 0 to howmany/10
            lru_add(l, payloads[j], (void *) payloads[j]);
        }
    }

    lru_destroy(l);
}

#endif
