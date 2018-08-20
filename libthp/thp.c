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

#include "thp.h"

/*  Local types */

// jobs entry
struct _thp_job_t
{
    thp_fun fun_p; // pointer to function thread as to run
};
typedef struct _thp_job_t thp_job_t;

#define THP_CHECK(f) if ((!f) || (f->magic != THP_MAGIC_ID)) return (THP_BAD_HANDLE);
#define THP_CHECK_BLOCK(f,b) if ((!f) || (!b) || (f->magic != THP_MAGIC_ID)) return (THP_WRONG_PARAM);

static void _thp_lock(thp_h *thp)
{
    pthread_mutex_lock(&(thp->mutex));
}

static void _thp_unlock(thp_h *thp)
{
    pthread_mutex_unlock(&(thp->mutex));
}

// allocate thp_job_t and set fun pointer value
static thp_job_t *_thp_job_create(thp_fun fun_p)
{
    thp_job_t *tj = malloc(sizeof(thp_job_t));
    tj->fun_p = fun_p;
    return tj;
}

// create a thread pool
thp_h *thp_create(thp_h *thp, int num_threads, int *err)
{
    // no preallocated handle, create one
    if (thp == NULL)
    {
        if ((thp = (void *) malloc(sizeof(thp_h))) == NULL)
        {
            *err = THP_NO_MEMORY;
            return (NULL);
        }
        thp->allocated = 1;
    }
    else
    {
        thp->allocated = 0;
    }

    thp->magic = CH_MAGIC_ID;
    if ( ch_create(&thp->in_queue, CH_DATALEN_VOIDP) == NULL )
    {
        *err = THP_INTERNAL_ERROR_CH_CREATE;
        return NULL;
    }

    if ( ch_create(&thp->wait_queue, CH_DATALEN_VOIDP) == NULL )
    {
        *err = THP_INTERNAL_ERROR_CH_CREATE;
        return NULL;
    }
    thp->wait_calls = 0;
    thp->to_be_waited = 0;
    thp->submitted = 0;
    thp->num_threads = num_threads;

    pthread_mutex_init(&(thp->mutex), NULL);

    // now start num_threads threads

    for (int i=0; i < num_threads; i++)
    {

    }

    return thp;
}

// add work to thread pool, return current number of jobs pushed
int thp_add(thp_h *thp, thp_fun fun_p)
{
    // create thp_job
    thp_job_t *tj = _thp_job_create(fun_p);
    // enque it

    _thp_lock(thp);
    int rc = ch_put(&thp->in_queue, tj);
    if ( rc < 0 )
    {
        _thp_unlock(thp);
        return rc;
    }

    thp->submitted++;
    thp->to_be_waited++;
    _thp_unlock(thp);

    return thp->submitted;
}

// wait for work to complete
void thp_wait(thp_h *thp)
{
    thp_job_t *tj;

    _thp_lock(thp);
    // nothing to wait for
    if ( thp->to_be_waited == 0 )
    {
        _thp_unlock(thp);
        return;
    }
    while (thp->to_be_waited)
    {
        ch_get(&thp->wait_queue, &tj);
        thp->to_be_waited--;
    }
    return;
}

// cleanup and free the pool
void thp_destroy(thp_h *thp )
{
    // send EOT for each thread;
    for (int i =0; i<thp->num_threads; i++)
    {
        ch_put(&thp->in_queue, CH_ENDOFTRANSMISSION);
    }
    // all the threads should terminate now

    // wait for all threads if any
    thp_wait(thp);

    ch_destroy(&thp->in_queue);
    ch_destroy(&thp->wait_queue);

    pthread_mutex_destroy(&(thp->mutex));
    if (thp->allocated)
    {
        free(thp);
    }

    return;
}
