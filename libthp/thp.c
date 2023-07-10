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
    thp_fun fun; // pointer to function thread as to run
    void *fun_param; // param to pass to function
};
typedef struct _thp_job_t thp_job_t;

#define THP_CHECK(f) if ((!f) || (f->magic != THP_MAGIC_ID)) return (THP_BAD_HANDLE);
#define THP_CHECK_BLOCK(f, b) if ((!f) || (!b) || (f->magic != THP_MAGIC_ID)) return (THP_WRONG_PARAM);

static void _thp_lock(thp_h *thp)
{
    pthread_mutex_lock(&(thp->mutex));
}

static void _thp_unlock(thp_h *thp)
{
    pthread_mutex_unlock(&(thp->mutex));
}

// allocate thp_job_t and set fun pointer value
static thp_job_t *_thp_job_create(thp_fun fun_p, void *arg)
{
    thp_job_t *tj = malloc(sizeof(thp_job_t));
    if ( tj == NULL )
    {
        return NULL;
    }
    tj->fun = fun_p;
    tj->fun_param = arg;
    return tj;
}

static void *thp_queue_runner(void *thp)
{
    thp_h *t = (thp_h *) thp;
    thp_job_t *tj = NULL;
    int rc;

    while (( rc = ch_get(&t->in_queue, &tj)) != CH_GET_ENDOFTRANSMISSION )
    {
        // execute job
        if (rc == CH_OK)
        {
            tj->fun(tj->fun_param);
            // pass job to wait queue
            ch_put(&t->wait_queue, tj);
        }
    }
    // got END of CH_GET_ENDOFTRANSMISSION
    // terminate
    return 0;
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
    thp->wait_running = 0;
    thp->to_be_waited = 0;
    thp->submitted = 0;
    thp->num_threads = num_threads;

    pthread_mutex_init(&(thp->mutex), NULL);

    // allocate space for pthread_t ids - TODO check calloc return
    thp->threads = calloc(num_threads, sizeof(pthread_t));

    // now start num_threads threads
    for (int i=0; i < num_threads; i++)
    {
        // start thread and keep pthread_ids;
        pthread_create(&thp->threads[i], NULL, &thp_queue_runner, thp);
    }

    return thp;
}

// add work to thread pool, return current number of jobs pushed
int thp_add(thp_h *thp, thp_fun fun_p, void *arg)
{
    // create thp_job
    thp_job_t *tj = _thp_job_create(fun_p, arg);

    if ( tj == NULL )
    {
        return THP_NO_MEMORY;
    }
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
int thp_wait(thp_h *thp)
{
    thp_job_t *tj;
    int count = 0;

    _thp_lock(thp);
    if (thp->wait_running != 0 )
    {
        // thp_wait already running, only one at a time
        return THP_TOO_MANY_WAIT;
    }
    thp->wait_running = 1;
    // nothing to wait for
    if ( thp->to_be_waited == 0 )
    {
        thp->wait_running = 0;
        _thp_unlock(thp);
        return 0;
    }
    while (thp->to_be_waited)
    {
        if ( ch_get(&thp->wait_queue, &tj) == CH_OK )
        {
            free(tj);
            thp->to_be_waited--;
            count++;
        }
    }
    thp->wait_running = 0;
    _thp_unlock(thp);
    return count;
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

    void *retval;
    for (int i =0; i<thp->num_threads; i++)
    {
        pthread_join(thp->threads[i], &retval);
    }

    // wait for all threads if any
    thp_wait(thp);

    ch_destroy(&thp->in_queue);
    ch_destroy(&thp->wait_queue);

    pthread_mutex_destroy(&(thp->mutex));
    free(thp->threads);
    if (thp->allocated)
    {
        free(thp);
    }

    return;
}
