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

#define _XOPEN_SOURCE 700


#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "ll.h"

// ll_create : create lru list object (ll)
// ll is basically a double linked list with a top and bottom pointers which
// exposes the node element and holds a key and a payload data. key is not used
// in ll

ll_t    *ll_create(int dim)
{
    ll_t *ll = NULL;
    // allocate ll_t
    ll = (ll_t *) malloc(sizeof(ll_t));
    if (ll == NULL)
    {
        return (NULL);
    }

    ll->magic = LL_MAGIC_ID;
    ll->size = dim;
    ll->top = ll->last = NULL;

    // allocate all slot entries and put them on the freelist, linked

    ll->save = ll->freelist = calloc(dim, sizeof(ll_slot_t));
    if ( ll->freelist == NULL )
    {
        free(ll);
        return NULL;
    }

    ll->fl_size = dim;

    // init ll critical region mutex
#ifdef MUTEX_CHECKS
    pthread_mutexattr_t mta;
    pthread_mutexattr_init(&mta);
    pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&(ll->lock), &mta);
#else
    pthread_mutex_init(&(ll->lock), NULL);
#endif

    for (int i=0; i<dim; i++)
    {
        if (i == (dim-1))
        {
            ll->freelist[i].next = NULL;
        }
        else
        {
            ll->freelist[i].next = &(ll->freelist[i+1]);
        }
    }
    return ll;
}

static void _ll_lock(ll_t *ll)
{
    int ret = pthread_mutex_lock(&(ll->lock));
#ifdef MUTEX_CHECKS
    if (ret != 0)
    {
        printf("_ll_lock() - pthread_mutex_lock() returned %d\n", ret);
        abort();
    }
#endif
}

static void _ll_unlock(ll_t *ll)
{
    int ret = pthread_mutex_unlock(&(ll->lock));
#ifdef MUTEX_CHECKS
    if (ret != 0)
    {
        printf("_ll_unlock() - pthread_mutex_unlock() returned %d\n", ret);
        abort();
    }
#endif
}

// ll_slot_new : alloc a slot i.e. get it from freelis of preallocated slots
ll_slot_t *ll_slot_new(ll_t *ll) // gets a slot from the freelist
{
    if ( ll == NULL ) return NULL;

    _ll_lock(ll);

    if (ll->freelist == NULL)
    {
        _ll_unlock(ll);
        return NULL;
    }

    ll_slot_t *ret = ll->freelist;
    ll->freelist = ret->next;

    // clear before returning
    ret->prev = ret->next = ret->payload = NULL;

    _ll_unlock(ll);

    return ret;
}

// ll_slot_free : free a slot i.e. put it back in freelist

void ll_slot_free(ll_t *ll, ll_slot_t *slot) // puts slot back in freelist
{
    if ( ll == NULL ) return;

    _ll_lock(ll);
    ll_slot_t *oldfirst = ll->freelist;
    ll->freelist = slot;
    slot->next = oldfirst;
    _ll_unlock(ll);

}

// removes last ll_slot, returns payload, puts slot back to freelist
ll_slot_t *ll_remove_last(ll_t *ll, void **payload)
{
    if ( ll == NULL )
    {
        //printf("ll(%p) == null or ll->last(%p) == null\n", ll, ll->last);
        return NULL;
    }

    _ll_lock(ll);

    if ( ll->last == NULL )
    {
        // nothing to remove
        _ll_unlock(ll);
        return(NULL);
    }

    ll_slot_t *oldlast = ll->last;
    // copy out data
    *payload = ll->last->payload;
    // now remove entry
    ll->last = oldlast->next;

    if ( ll->last != NULL )
    {
        // fixing prev of new last
        ll->last->prev = NULL;
    }
    else
    {
        // removing the last element
        ll->top = NULL;
    }
    _ll_unlock(ll);

    // now we have a dangling oldlast to put back to freelist
    ll_slot_free(ll, oldlast);
    return oldlast;
}

// take a slot that might be either in list or just taken from freelist
// and put it to top

void ll_slot_move_to_top(ll_t *ll, ll_slot_t *slot)
{
    if ( ll == NULL)
    {
        printf("ll(%p) == null\n", ll);
        return;
    }

    _ll_lock(ll);
    ll_slot_t *oldtop = ll->top;

    // trying to move to top slot already at top
    if ( ll->top == slot)
    {
        _ll_unlock(ll);
        return;
    }

    // new slot ( or only slot )to put at top
    if ((slot->prev == NULL) && (slot->next == NULL))
    {
        ll->top = slot;
        if ( oldtop != NULL )
        {
            oldtop->next = slot;
            slot->prev = oldtop;
        }
        // no last set ?
        if ( ll->last == NULL)
        {
            ll->last = slot;
        }
        _ll_unlock(ll);
        return;
    }

    // moving a member in the middle of list
    if ((slot->next != NULL) && (slot->prev != NULL ))
    {
        // reconnecting
        slot->prev->next = slot->next;
        slot->next->prev = slot->prev;

        // setting to top
        ll->top = slot;
        slot->next = NULL;
        if ( oldtop != NULL )
        {
            oldtop->next = slot;
            slot->prev = oldtop;
        }
        _ll_unlock(ll);
        return;
    }

    // moving last to top
    //if ( ll->last == slot ) // or slot->prev == NULL ???
    if ( slot->prev == NULL ) // or slot->prev == NULL ???
    {
        // this should be last!!!
        if ( ll->last != slot )
        {
            printf("slot->prev == NULL and ll->last != slot\n");
            //assert(ll->last == slot);
            abort();
        }
        // reconnecting
        slot->next->prev = NULL;
        ll->last = slot->next;

        // setting to top
        ll->top = slot;
        slot->next = NULL;
        if ( oldtop != NULL )
        {
            oldtop->next = slot;
            slot->prev = oldtop;
        }
        _ll_unlock(ll);
        return;
    }

    if ( slot->next == NULL ) // this is already top
    {
        // this should be top!!!
        if ( ll->top != slot )
        {
            printf("slot->next == NULL and ll->top != slot\n");
            //assert(ll->last == slot);
            abort();
        }
        // nothing to do : already top item
    }

    _ll_unlock(ll);
    return;
}

// ll_slot_set_payload : sets payload to a slot
void ll_slot_set_payload(ll_t *ll, ll_slot_t *slot, void *payload)
{
    slot->payload = payload;
}

// destroy object, using original calloc saved pointer
int ll_destroy(ll_t *ll)
{
    if (ll)
    {
        free(ll->save);
        pthread_mutex_destroy(&ll->lock);
        free(ll);
    }
    return LL_OK;
}


void ll_print(ll_t *ll, int (*payload_print)(void *))
{
    ll_slot_t *l = ll->last;
    int i=0;

    printf("top %p, last %p\n", ll->top, ll->last);

    if (ll->last->prev != NULL)
    {
        printf("inconsistency on last slot\n");
    }

    if (ll->top->next != NULL)
    {
        printf("inconsistency on top slot\n");
    }

    while (l != NULL)
    {
        printf("[%p] : next %p, prev %p\n", l, l->next, l->prev);
        printf("[%p] : payload : ",l); payload_print(l->payload);
        l = l->next;
        i++;
        if (i>50) break;
    }
}


#ifdef TEST


static void check_cons(ll_t *ll, int count)
{
    ll_slot_t *l = ll->last;
    int size = 0;


    if (ll->last->prev != NULL)
    {
        printf("inconsistency on last slot\n");
        exit(1);

    }

    if (ll->top->next != NULL)
    {
        printf("inconsistency on top slot\n");
        exit(1);

    }

    while (l != NULL)
    {
        l = l->next;
        size++;
    }

    if (size != count)
    {
        printf("size %d, count %d\n", size, count);
        exit(1);
    }

}



int main(int argc, char **argv)
{
    if ( argc < 2 )
    {
        printf("usage : %s <llsize> [<loops>]\n", argv[0]);
        exit(1);
    }
    void *payload;
    int howmany = atoi(argv[1]);
    int loops = 1;
    if ( argv[2] )
    {
        loops = atoi(argv[2]);
    }

    ll_slot_t *all[howmany];
    ll_t *ll;

    for (int k=0; k<loops; k++)
    {
        ll = ll_create(howmany);
        printf("ll_create(%d) ret %p\n", howmany, ll);

        for (int i =0; i<howmany; i++)
        {
            ll_slot_t *n = ll_slot_new(ll);
            if ( n == NULL )
            {
                printf("ll_slot_new ret NULL\n");
                // put back last
                ll_remove_last(ll, &payload);
                printf("ll_removed_last %s\n", (char *) payload);
                n = ll_slot_new(ll);
            }
            all[i] = n;
            //printf("ll_slot_new ret %p\n", n);

        #ifdef NO_ALLOC
            n->payload = "bazinga";
        #else
            char *str = malloc(100);
            sprintf(str, "payload%d", i);
            n->payload = str;
        #endif
            ll_slot_move_to_top(ll, n);
            //printf("last now is %s\n", (char *) ll->last->payload);
            //printf("top now is %s\n", (char *) ll->top->payload);
        }

        check_cons(ll, howmany);

        printf("===== moving to top ascending\n");

        for (int i=0; i< howmany; i++)
        {
            //printf("last now is %s, last->next %s\n", (char *) ll->last->payload, (char *)ll->last->next->payload);
            //printf("----- setting to top all[%d] = %s\n", i, (char *) all[i]->payload);
            ll_slot_move_to_top(ll, all[i]);
            //printf("last now is %s, last->next %s\n", (char *) ll->last->payload, (char *)ll->last->next->payload);
            //printf("top now is %s\n", (char *) ll->top->payload);
        }
        check_cons(ll, howmany);

        printf("===== moving to top descending\n");

        for (int i=howmany-1; i>=0; i--)
        {
            //printf("last now is %s, last->next %s\n", (char *) ll->last->payload, (char *)ll->last->next->payload);
            //printf("----- setting to top all[%d] = %s\n", i, (char *) all[i]->payload);
            ll_slot_move_to_top(ll, all[i]);
            //printf("last now is %s, last->next %s\n", (char *) ll->last->payload, (char *)ll->last->next->payload);
            //printf("top now is %s\n", (char *) ll->top->payload);
        }

        check_cons(ll, howmany);

        printf("===== random move top\n");

        for (int i=0; i< howmany*100; i++)
        {
            int j = rand() % howmany;
            ll_slot_move_to_top(ll, all[j]);
        }

#ifdef TRACE
        printf("===== print all data\n");

        for (int i=0; i< howmany; i++)
        {
            printf("all[%d] payload %s, next %s, prev %s\n", i, (char *) all[i]->payload,
                   (char *) ((all[i]->next) ? all[i]->next->payload : "null"),
                   (char *) ((all[i]->prev) ? all[i]->prev->payload : "null")
                   );
        }
#endif

        check_cons(ll, howmany);

        ll_destroy(ll);

    }


    printf("Test passed!\n");
}

#endif
