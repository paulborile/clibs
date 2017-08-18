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
#define LL_SLOT_IN_FREELIST 0xFE
#define LL_SLOT_DANGLING 0xFD // not in ll list nor in freelist


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

ll_t    *ll_create(int dim, int payload_size)
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
        ll->freelist[i].payload = malloc(payload_size);
        if ( ll->freelist[i].payload == NULL )
        {
            free(ll);
            return NULL;
        }
        ll->freelist[i].status = LL_SLOT_IN_FREELIST;
    }
    return ll;
}

// print slot contents
static void ll_print_slot(ll_t *ll, ll_slot_t *slot)
{
    printf("slot = %p, status = %d, prev = %p, next = %p, payload = %p\n\tll->top = %p, ll->last = %p, ll->freelist = %p\n",
           slot, slot->status, slot->prev, slot->next, slot->payload, ll->top, ll->last, ll->freelist);
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
ll_slot_t *ll_slot_new(ll_t *ll, void **payload) // gets a slot from the freelist
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
    ret->prev = ret->next = NULL;
    ret->status = LL_SLOT_DANGLING;
    *payload = ret->payload;

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
    slot->status = LL_SLOT_IN_FREELIST;
    _ll_unlock(ll);

}

// _ll_slot_free : free a slot i.e. put it back in freelist no locks

static void _ll_slot_free(ll_t *ll, ll_slot_t *slot) // puts slot back in freelist
{
    if ( ll == NULL ) return;

    ll_slot_t *oldfirst = ll->freelist;
    ll->freelist = slot;
    slot->next = oldfirst;
    slot->status = LL_SLOT_IN_FREELIST;
}


// removes last ll_slot, returns payload
ll_slot_t *ll_remove_last(ll_t *ll, void **payload)
{
    if ( ll == NULL )
    {
        //printf("ll(%p) == null or ll->last(%p) == null\n", ll, ll->last);
        return NULL;
    }

    _ll_lock(ll);

    // empty ll
    if ( ll->last == NULL )
    {
        // nothing to remove
        _ll_unlock(ll);
        return(NULL);
    }

    // check for integrity : we may remove
    if ( ll->last->prev != NULL )
    {
        ll_slot_t *slot = ll->last;
        printf("remove_last() - NOT LAST : ll->last point to slot with valid prev\n");
        ll_print_slot(ll, slot);

        //assert(ll->last == slot);
        abort();
    }

    // there is a last, save it
    ll_slot_t *oldlast = ll->last;
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

    // copy out data
    *payload = oldlast->payload;
    // now we have a dangling oldlast : caller will take care of putting it in freelist again after cleanup
    oldlast->status = LL_SLOT_DANGLING;

    _ll_unlock(ll);
    return oldlast;
}

// add a slot and put it to top

void ll_slot_add_to_top(ll_t *ll, ll_slot_t *slot)
{
    if ( ll == NULL)
    {
        printf("ll(%p) == null\n", ll);
        return;
    }

    _ll_lock(ll);

    // we can only add dangling slots
    if ( slot->status != LL_SLOT_DANGLING )
    {
        printf("ll_slot_add_to_top() - adding a non dangling slot\n");
        ll_print_slot(ll, slot);
        _ll_unlock(ll);
        return;
    }

    // checks
    if (( slot->prev != NULL) || (slot->next != NULL))
    {
        printf("ll_slot_add_to_top() - prev and next are not NULL\n");
        ll_print_slot(ll, slot);
        _ll_unlock(ll);
        return;
    }

    ll_slot_t *oldtop = ll->top;
    // adding slot ( or only slot )to put at top
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

// move an existing slot to top
void ll_slot_move_to_top(ll_t *ll, ll_slot_t *slot)
{
    if ( ll == NULL)
    {
        printf("ll(%p) == null\n", ll);
        return;
    }

    _ll_lock(ll);

    // avoid moving to top slots in freelist or with null/dangling payload
    // this may happen with heavy multi thread activity and small sizes : a lru_check attempts to move to top
    // a slot which has already been removed and put into freelist and/or reallocated

    if (( slot->status == LL_SLOT_IN_FREELIST ) || ( slot->status == LL_SLOT_DANGLING ))
    {
        _ll_unlock(ll);
        return;
    }
    // trying to move to top slot already at top
    if ( ll->top == slot)
    {
        _ll_unlock(ll);
        return;
    }

    ll_slot_t *oldtop = ll->top;
    // adding slot ( or only slot )to put at top
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
        // extracting and reconnecting list
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
            printf("move_to_top() - NOT LAST : slot->prev == NULL and ll->last != slot\n");
            ll_print_slot(ll, slot);
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

    // moving topmost to top
    if ( slot->next == NULL ) // this is already top
    {
        // this should be top!!!
        if ( ll->top != slot )
        {
            printf("move_to_top() - NOT TOP : slot->next == NULL and ll->top != slot\n");
            ll_print_slot(ll, slot);
            //assert(ll->last == slot);
            abort();
        }
        // nothing to do : already top item
    }

    _ll_unlock(ll);
    return;
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

// slow! test only! idx == 0 is the most recently used.
int ll_get_payload(ll_t *ll, int idx, void** payload)
{

    // printf("payload: %x\n", payload);

    if (ll->last->prev != NULL)
    {
        printf("inconsistency on last slot\n");
        return LL_ERROR_LAST_SLOT_INCONSISTENCY;
    }

    if (ll->top->next != NULL)
    {
        printf("inconsistency on top slot\n");
        return LL_ERROR_TOP_SLOT_INCONSISTENCY;
    }

    ll_slot_t *l = ll->top;
    int i = 0;

    // printf("top %p, last %p\n", ll->top, ll->last);

    while (l != NULL && i < idx)
    {
        // printf("[%p] : next %p, prev %p\n", l, l->next, l->prev);
        // printf("[%p] : payload : ",l); payload_print(l->payload);
        l = l->prev;
        i++;
    }

    if(l == NULL)
    {
        return LL_ERROR_INDEX_OUT_OF_RANGE;
    }

    (*payload) = l->payload;

    return LL_OK;
}

void ll_print(ll_t *ll, int (*payload_print)(void *))
{
    ll_slot_t *l = ll->last;
    int i=0;

    printf("top %p, last %p\n", ll->top, ll->last);

    if ((ll->last != NULL) && (ll->last->prev != NULL))
    {
        printf("inconsistency on last slot\n");
    }

    if ((ll->top != NULL) && (ll->top->next != NULL))
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
