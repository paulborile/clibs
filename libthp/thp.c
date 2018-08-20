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
thp_h *thp_create(thp_h *thp, int num_threads)
{
    // no preallocated handle, create one
    if (thp == NULL)
    {
        if ((thp = (void *) malloc(sizeof(thp_h))) == NULL)
        {
            return (NULL);
        }
        thp->allocated = 1;
    }
    else
    {
        thp->allocated = 0;
    }

    thp->magic = CH_MAGIC_ID;
    ch_create(&thp->in_queue, CH_DATALEN_VOIDP);
    ch_create(&thp->wait_queue, CH_DATALEN_VOIDP);
    thp->wait_calls = 0;
    thp->submitted = 0;

    pthread_mutex_init(&(thp->mutex), NULL);

    return thp;
}

// add work to thread pool, return current number of jobs pushed
int thp_add(thp_h *thp, thp_fun fun_p)
{
    /*
       // create thp_job
       thp_job_t *tj = _thp_job_create(fun_p);
       // enque it
       ch_put(thp->in_queue, tj);
     */
    return 1;
}

// wait for work to complete
void thp_wait(thp_h *thp)
{
    /*
       while (count++ < thp->submitted)
       {
       ch_get(wait_queue, );
       to_be_waited--;
       }
       return;
     */
    return;
}

// cleanup and free the pool
void thp_destroy(thp_h *thp )
{
    ch_destroy(&thp->in_queue);
    ch_destroy(&thp->wait_queue);

    pthread_mutex_destroy(&(thp->mutex));

    if (thp->allocated)
    {
        free(thp);
    }

    return;
}
