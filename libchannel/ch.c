/*
   Copyright (c) 2003, Paul Stephen Borile
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

#include "ch.h"

/*  Local types                                 */

#define CH_CHECK(f) if ((!f) || (f->magic != CH_MAGIC_ID)) return (CH_BAD_HANDLE);
#define CH_CHECK_BLOCK(f,b) if ((!f) || (!b) || (f->magic != CH_MAGIC_ID)) return (CH_BAD_HANDLE);
#define CH_CHECK_VOID(f,b) if ((!f) || (!b) || (f->magic != CH_MAGIC_ID)) return (NULL);

static void _ch_lock(ch_h *ch)
{
    pthread_mutex_lock(&(ch->ch_mutex));
}

static void _ch_unlock(ch_h *ch)
{
    pthread_mutex_unlock(&(ch->ch_mutex));
}

/*
 * channel is golang channel inspired communication fifo channel
 * multithread aware. Put() are non blocking and data is copied into the channel
 * while Get() can be blocking or non blocking. Use it to synch threads together
 */

void *ch_create(ch_h *ch, int datalen)
{
    // no preallocated handle, create one
    if (ch == NULL)
    {
        if ((ch = (void *) malloc(sizeof(ch_h))) == NULL)
        {
            return (NULL);
        }
        ch->allocated = 1;
    }
    else
    {
        ch->allocated = 0
    }

    ch->magic = CH_MAGIC_ID;
    ch->datalen = datalen;
    ch->count = 0;
    ch->attr = CH_ATTR_BLOCKING_GET;
    ch->head = NULL;
    ch->tail = NULL;
    ch->waiting_threads = 0;

    // ch->free_fun = free_fun;

    pthread_mutex_init(&(ch->ch_mutex), NULL);
    return ch;
}

#define CH_PUT_STD 1
#define CH_PUT_TOP 2

static int _ch_put(ch_h *ch, void *block, int where)
{
    ch_elem_t *element = NULL;

    CH_CHECK_BLOCK(ch, block);

    if ((element = (void *) malloc(sizeof(ch_elem_t))) == NULL)
    {
        return (CH_NO_MEMORY);
    }
    element->prev = NULL;

    if ( block == CH_ENDOFTRANSMISSION )
    {
        element->block = CH_ENDOFTRANSMISSION;
    }
    else
    {
        if (ch->datalen > 0)
        {
            if (( element->block = malloc(ch->datalen)) == NULL)
            {
                return (CH_NO_MEMORY);
            }
            memcpy(element->block, block, ch->datalen);
        }
        else if ( ch->datalen == CH_DATALEN_VOIDP )
        {
            //just copy poiters
            element->block = block;
        }
        else if ( ch->datalen == CH_DATALEN_STRING )
        {
            if (( element->block = malloc(strlen(block)+1)) == NULL)
            {
                return (CH_NO_MEMORY);
            }
            strcpy(element->block, block);
        }
    }

    _ch_lock(ch);

    if ( where == CH_PUT_STD )
    {
        if (ch->count == 0)
        {
            ch->head = element;
        }
        else
        {
            ch->tail->prev = element;
        }
        ch->tail = element;
    }
    else
    {
        if (ch->count == 0)
        {
            ch->tail = element;
        }
        else
        {
            element->prev = ch->head;
        }
        ch->head = element;
    }
    ch->count++;

    // check if threads blocked in get notify them
    if ((ch->attr & CH_ATTR_BLOCKING_GET) == CH_ATTR_BLOCKING_GET)
    {
        if (ch->waiting_threads != 0)
        {
            pthread_cond_signal(&(ch->ch_condvar));
        }
    }

    int ccount = ch->count;
    _ch_unlock(ch);
    return (ccount);
}

// put at end of queue
int ch_put(ch_h *ch, void *block)
{
    return _ch_put(ch, block, CH_PUT_STD);
}

// skip the queue and put on top
int ch_put_head(ch_h *ch, void *block)
{
    return _ch_put(ch, block, CH_PUT_TOP);
}

// get from top of list - if CH_ATTR_BLOCKING_GET set block until element arrives.
void *ch_get(ch_h *ch, void *block)
{
    ch_elem_t *element = NULL;

    CH_CHECK_VOID(ch, block);

    _ch_lock(ch);

    if (ch->count == 0)
    {
        if ((ch->attr & CH_ATTR_BLOCKING_GET) == CH_ATTR_BLOCKING_GET)
        {
            ch->waiting_threads++;

            pthread_cond_wait(&(ch->ch_condvar), &(ch->ch_mutex));
        }
        else
        {
            _ch_unlock(ch);
            return NULL;
        }
    }

    if ((ch->attr & CH_ATTR_BLOCKING_GET) == CH_ATTR_BLOCKING_GET)
    {
        ch->waiting_threads--;

        if (ch->count == 0)
        {
            // fifo empty after pthread_cond_signal
            // some error condition to check
            _ch_unlock(ch);
            return NULL;
        }
    }

    element = ch->head;

    if (element->block == CH_ENDOFTRANSMISSION)
    {
        // special end of transmission signal
        element->block = NULL;
        free(element);

        ch->count--;
        _ch_unlock(ch);
        return (block);
    }

    if (ch->datalen > 0)
    {
        memcpy(block, element->block, ch->datalen);
    }
    else if ( ch->datalen == CH_DATALEN_VOIDP )
    {
        //just copy poiters
        block = element->block;
    }
    else if ( ch->datalen == CH_DATALEN_STRING )
    {
        strcpy(block, element->block);
    }

    // detach top element
    ch->head = ch->head->prev;

    if (ch->head == NULL)
    {
        ch->tail = NULL;
    }

    free(element->block);
    element->block = NULL;
    free(element);

    ch->count--;
    _ch_unlock(ch);
    return (block);
}

// look at first without dequeing
void *ch_peek(ch_h *ch, void *block)
{
    ch_elem_t *element = NULL;

    CH_CHECK_VOID(ch, block);

    _ch_lock(ch);

    if (ch->count == 0)
    {
        _ch_unlock(ch);
        return NULL;
    }

    element = ch->head;
    if (element->block == CH_ENDOFTRANSMISSION)
    {
        // special end of transmission signal
        ch->count--;
        _ch_unlock(ch);
        return (block);
    }

    if (ch->datalen > 0)
    {
        memcpy(block, element->block, ch->datalen);
    }
    else if ( ch->datalen == CH_DATALEN_VOIDP )
    {
        //just copy poiters
        block = element->block;
    }
    else if ( ch->datalen == CH_DATALEN_STRING )
    {
        strcpy(block, element->block);
    }
    _ch_unlock(ch);
    return (block);
}

// set attributes
int ch_setattr(ch_h *ch, int attr, int val)
{
    CH_CHECK(ch);

    switch (attr)
    {
    case CH_BLOCKING_MODE:

        if (val != CH_ATTR_NON_BLOCKING_GET && val != CH_ATTR_BLOCKING_GET)
        {
            return (CH_WRONG_ATTR);
        }

        ch->attr &= val;

        if ((ch->attr & CH_ATTR_BLOCKING_GET) == CH_ATTR_BLOCKING_GET)
        {
            pthread_cond_init(&(ch->ch_condvar), NULL);
        }

        break;

    default:
        return (CH_WRONG_ATTR);
    }

    return (0);
}

// get value for attributes and channel size
int ch_getattr(ch_h *ch, int attr, int *val)
{
    CH_CHECK(ch);

    switch (attr)
    {
    case CH_BLOCKING_MODE:
        *val = ch->attr;
        break;
    case CH_COUNT:
        _ch_lock(ch);
        *val = ch->count;
        _ch_unlock(ch);
        break;
    default:
        return (CH_WRONG_ATTR);
    }
    return (0);
}

// emtpy the channel without signalling ayone
int ch_clean(ch_h *ch)
{
    ch_elem_t *element = NULL;
    ch_elem_t *cur_element = NULL;

    CH_CHECK(ch);
    _ch_lock(ch);

    if (ch->count != 0)
    {
        element = ch->head;
        cur_element = ch->head->prev;

        while (element != NULL)
        {
            free(element->block);
            element->block = NULL;
            free(element);

            if (cur_element != NULL)
            {
                element = cur_element;
                cur_element = cur_element->prev;
            }
            else
            {
                element = NULL;
                ch->head = NULL;
                ch->tail = NULL;
            }
        }
    }

    ch->count = 0;

    _ch_unlock(ch);
    return CH_OK;
}

// destroys and signals waiting threads
int ch_destroy(ch_h *ch)
{
    ch_elem_t *element = NULL;
    ch_elem_t *cur_element = NULL;

    CH_CHECK(ch);
    ch_clean(ch);

    ch->magic = 0xD;
    // wakeup sleeping threads
    if ((ch->attr & CH_ATTR_BLOCKING_GET) == CH_ATTR_BLOCKING_GET)
    {
        while (ch->waiting_threads > 0)
        {
            pthread_cond_broadcast(&(ch->ch_condvar));
            //sleep(1);
        }
    }

    pthread_mutex_destroy(&(ch->ch_mutex));
    if (ch->allocated)
    {
        free(ch);
    }
    return CH_OK;
}
