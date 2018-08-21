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

#include "ch.h"

#ifndef THP_H
#define THP_H

#ifdef __cplusplus
extern "C" {
#endif

/** hash magic */
#define THP_MAGIC_ID         0xFEAD

#define THP_OK                1   // No error
#define THP_BAD_HANDLE       -1  // handle null of not pointing to thp_
#define THP_WRONG_PARAM      -2
#define THP_TOO_MANY_WAIT    -3 // only one thp_wait() can be running at a time
#define THP_NO_MEMORY        -4 // only one thp_wait() can be running at a time
#define THP_INTERNAL_ERROR_CH_CREATE    -5

// thp object
struct _thp_h {
    int magic;
    ch_h in_queue; // jobs are queued here for execution
    ch_h wait_queue; // terminated jobs are queued here for wait
    int wait_running; // ensure that only 1 wait is running
    int submitted; // total number of submitted jobs
    int to_be_waited; // totale number of jobs to be currenlty waited for (size of wait_queue)
    pthread_mutex_t mutex; // protection for thp_h
    int num_threads; // number of threads created
    pthread_t *threads; // array of pthread_t elements
    int allocated; // tell if this handle has been allocated by ib or not.
};
typedef struct _thp_h thp_h;

// work function
typedef void * (*thp_fun)(void *arg);

// create a thread pool
thp_h *thp_create(thp_h *thp, int num_threads, int *err);
int thp_add(thp_h *thp, thp_fun fun_p, void *arg);
int thp_wait(thp_h *thp);
void thp_destroy(thp_h *thp );

#ifdef __cplusplus
}
#endif

#endif
