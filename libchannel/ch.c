/*
   Copyright (c) 2003-, Paul Stephen Borile
   All rights reserved.
   License : MIT

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
#define CH_CHECK_BLOCK(f, b) if ((!f) || (!b) || (f->magic != CH_MAGIC_ID)) return (CH_WRONG_PARAM);

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
        ch->allocated = 0;
    }

    ch->magic = CH_MAGIC_ID;
    ch->datalen = datalen;
    ch->count = 0;
    ch->max_size = 0; // 0 = infinite
    ch->attr = 0;
    ch->attr |= CH_ATTR_BLOCKING_GETPUT;
    ch->head = NULL;
    ch->tail = NULL;
    ch->get_waiting_threads = 0;
    ch->put_waiting_threads = 0;

    pthread_mutex_init(&(ch->ch_mutex), NULL);
    pthread_cond_init(&(ch->ch_get_condvar), NULL);
    pthread_cond_init(&(ch->ch_put_condvar), NULL);
    return ch;
}

#define CH_PUT_STD 1
#define CH_PUT_TOP 2

// put at <where> of queue
// if where == CH_PUT_STD put at end of queue
// if where == CH_PUT_TOP put at top of queue
// if max_size!= 0 and queue is full wait on condition variable (if CH_BLOCKING_MODE set)
// if CH_BLOCKING_MODE not set and queue is full return CH_FULL
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

    if ((ch->max_size > 0) && ((ch->attr & CH_ATTR_BLOCKING_GETPUT) != CH_ATTR_BLOCKING_GETPUT))
    {
        if (ch->count >= ch->max_size)
        {
            // channel is full and NOT in blocking mode so we need to return
            // free element just allocated above
            if ((ch->datalen != CH_DATALEN_VOIDP) && (block != CH_ENDOFTRANSMISSION))
            {
                free(element->block);
            }
            free(element);

            _ch_unlock(ch);
            return CH_PUT_CHANNEL_FULL;
        }
    }

    if ( where == CH_PUT_STD )
    {
        if (ch->count == 0)
        {
            ch->head = element;
        }
        else
        {
            if (ch->max_size != 0)
            {
                while (ch->count >= ch->max_size)
                {
                    // wait on condition variable for channel to drain
                    ch->put_waiting_threads++;
                    pthread_cond_wait(&(ch->ch_put_condvar), &(ch->ch_mutex));
                    ch->put_waiting_threads--;
                }
            }
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
            if (ch->max_size != 0)
            {
                while (ch->count >= ch->max_size)
                {
                    // wait on condition variable for channel to drain
                    ch->put_waiting_threads++;
                    pthread_cond_wait(&(ch->ch_put_condvar), &(ch->ch_mutex));
                    ch->put_waiting_threads--;
                }
            }

            element->prev = ch->head;
        }
        ch->head = element;
    }
    ch->count++;

    // check if threads blocked in get notify them
    if ((ch->attr & CH_ATTR_BLOCKING_GETPUT) == CH_ATTR_BLOCKING_GETPUT)
    {
        pthread_cond_signal(&(ch->ch_get_condvar));
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
int ch_get(ch_h *ch, void *block)
{
    ch_elem_t *element = NULL;

    CH_CHECK_BLOCK(ch, block);

    _ch_lock(ch);

    if ((ch->count == 0) && ((ch->attr & CH_ATTR_BLOCKING_GETPUT) != CH_ATTR_BLOCKING_GETPUT))
    {
        // non blocking mode, if channel is emptry return CH_GET_NODATA
        _ch_unlock(ch);
        return CH_GET_NODATA;
    }

    while (ch->count == 0)
    {
        ch->get_waiting_threads++;
        pthread_cond_wait(&(ch->ch_get_condvar), &(ch->ch_mutex));
        ch->get_waiting_threads--;

        // if (ch->count == 0)
        // {
        //     // fifo empty after pthread_cond_signal
        //     // some error condition to check
        //     _ch_unlock(ch);
        //     return CH_GET_NODATA;
        // }
    }

    element = ch->head;

    if (element->block == CH_ENDOFTRANSMISSION)
    {
        // detach top element

        ch->head = ch->head->prev;

        if (ch->head == NULL)
        {
            ch->tail = NULL;
        }

        // special end of transmission signal
        element->block = NULL;
        free(element);

        ch->count--;

        // TODO : wakeup threads waiting in ch_put
        if (ch->max_size != 0)
        {
            // fixed size channel, wakeup all threads waiting in ch_put
            if ( ch->put_waiting_threads > 0 )
            {
                // signal thread
                pthread_cond_signal(&(ch->ch_put_condvar));
            }
        }

        _ch_unlock(ch);


        return CH_GET_ENDOFTRANSMISSION;
    }

    if (ch->datalen > 0)
    {
        memcpy(block, element->block, ch->datalen);
    }
    else if ( ch->datalen == CH_DATALEN_VOIDP )
    {
        void **fake = block;
        //just copy poiters
        *fake = element->block;
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

    if ( ch->datalen != CH_DATALEN_VOIDP )
    {
        free(element->block);
        element->block = NULL;
    }
    free(element);

    ch->count--;

    // TODO : wakeup threads waiting in ch_put
    if (ch->max_size != 0)
    {
        // fixed size channel, wakeup all threads waiting in ch_put
        if ( ch->put_waiting_threads > 0 )
        {
            // signal thread
            pthread_cond_signal(&(ch->ch_put_condvar));
        }
    }

    _ch_unlock(ch);
    return CH_OK;
}

// look at first without dequeing
int ch_peek(ch_h *ch, void *block)
{
    ch_elem_t *element = NULL;

    CH_CHECK_BLOCK(ch, block);

    _ch_lock(ch);

    if (ch->count == 0)
    {
        _ch_unlock(ch);
        return CH_GET_NODATA;
    }

    element = ch->head;
    if (element->block == CH_ENDOFTRANSMISSION)
    {
        // special end of transmission signal
        ch->count--;
        _ch_unlock(ch);
        return CH_GET_ENDOFTRANSMISSION;
    }

    if (ch->datalen > 0)
    {
        memcpy(block, element->block, ch->datalen);
    }
    else if ( ch->datalen == CH_DATALEN_VOIDP )
    {
        void **fake = block;
        //just copy poiters
        *fake = element->block;
    }
    else if ( ch->datalen == CH_DATALEN_STRING )
    {
        strcpy(block, element->block);
    }
    _ch_unlock(ch);
    return CH_OK;
}

// set attributes
int ch_setattr(ch_h *ch, int attr, int val)
{
    CH_CHECK(ch);

    switch (attr)
    {
    case CH_BLOCKING_MODE:

        if (val != CH_ATTR_NON_BLOCKING_GETPUT && val != CH_ATTR_BLOCKING_GETPUT)
        {
            return (CH_WRONG_VALUE);
        }

        if (val == CH_ATTR_BLOCKING_GETPUT)
        {
            ch->attr |= CH_ATTR_BLOCKING_GETPUT;
        }
        else
        {
            ch->attr &= CH_ATTR_NON_BLOCKING_GETPUT;
        }

        ch->attr &= val;

        break;
    case CH_FIXED_SIZE:
        // val contains the size of the channel
        if (val >= 0)
        {
            ch->max_size = val;
        }

        break;
    default:
        return (CH_WRONG_ATTR);
    }

    return (CH_OK);
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
    case CH_FIXED_SIZE:
        *val = ch->max_size;
        break;
    default:
        return (CH_WRONG_ATTR);
    }
    return (CH_OK);
}

// emtpy the channel without signalling ayone
int ch_clean(ch_h *ch, ch_opaque_delete_func (*del_func))
{
    ch_elem_t *element = NULL;
    ch_elem_t *cur_element = NULL;

    CH_CHECK(ch);

    // User set del_func but hash table doesn't contain void pointers: set error to return (but still clean the table)
    if (del_func != NULL && ch->datalen != CH_DATALEN_VOIDP)
    {
        return CH_FREE_NOT_REQUESTED;
    }

    _ch_lock(ch);

    if (ch->count != 0)
    {
        element = ch->head;
        cur_element = ch->head->prev;

        // If channel transports pointers, element clean isn't needed
        while (element != NULL)
        {
            if ( element->block != CH_ENDOFTRANSMISSION )
            {
                if ( ch->datalen == CH_DATALEN_VOIDP )
                {
                    // If element block is a pointer, and caller provided a custom free function, use it
                    if (del_func != NULL)
                    {
                        del_func(element->block);
                    }
                }
                else
                {
                    free(element->block);
                    element->block = NULL;
                }
            }
            free(element);
            ch->count--;

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

    _ch_unlock(ch);
    return CH_OK;
}

// destroys and signals waiting threads
int ch_destroy(ch_h *ch)
{
    CH_CHECK(ch);
    ch_clean(ch, NULL);

    ch->magic = 0xD;
    // wakeup sleeping threads
    if ((ch->attr & CH_ATTR_BLOCKING_GETPUT) == CH_ATTR_BLOCKING_GETPUT)
    {
        while (ch->get_waiting_threads > 0)
        {
            pthread_cond_broadcast(&(ch->ch_get_condvar));
            //sleep(1);
        }
    }

    pthread_mutex_destroy(&(ch->ch_mutex));
    pthread_cond_destroy(&(ch->ch_get_condvar));

    if (ch->allocated)
    {
        free(ch);
    }
    return CH_OK;
}
