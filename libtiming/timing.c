/*
   Copyright (c) 2017, Paul Stephen Borile
   All rights reserved.
   License : MIT

 */

// for clock_gettime() and c99

#define _POSIX_C_SOURCE 199309L

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>


#include "timing.h"

struct timing_data
{
    struct  timeval start;
    struct  timeval end;
#ifdef  __linux__
    struct timespec nano_start, nano_end;
#endif
    int nano;
};

/*
 * timing_new_timer : create new time measure data
 */

void *timing_new_timer(int nanoprecision)
{
    struct timing_data *t;
    t = malloc(sizeof(struct timing_data));
    assert(t);
    t->nano = nanoprecision;
    return(t);
}

void timing_delete_timer(void *t)
{
    struct timing_data *tt = (struct timing_data *) t;
    free(tt);
}

/*
 * timing_start : call at beginning of event to measure
 */

// http://www.avrfreaks.net/forum/declaring-function-extern-inline-header-file
// removing the "inline" (which generates a warning)
inline void timing_start(void *t)
{
    struct timing_data *tt = (struct timing_data *) t;

#ifdef __linux__
    if ( tt->nano)
    {
        clock_gettime(CLOCK_MONOTONIC, &(tt->nano_start));
    }
    else
#endif
    {
        gettimeofday(&(tt->start), NULL);
    }
}

/*
 * timing_end : call at end of event to measure
 */

inline double timing_end(void *t)
{
    struct timing_data *tt = (struct timing_data *) t;
    double tstart, tend;
#ifdef __linux__
    if ( tt->nano)
    {
        clock_gettime(CLOCK_MONOTONIC, &(tt->nano_end));
        tstart = ((tt->nano_start.tv_sec * 1000000000) + tt->nano_start.tv_nsec);
        return (((tt->nano_end.tv_sec * 1000000000) + tt->nano_end.tv_nsec) - ((tt->nano_start.tv_sec * 1000000000) + tt->nano_start.tv_nsec));
    }
    else
#endif
    {
        gettimeofday(&(tt->end), NULL);
        return (((tt->end.tv_sec * 1000000) + tt->end.tv_usec) - ((tt->start.tv_sec * 1000000) + tt->start.tv_usec));
    }
}

#ifdef TEST

/*
 * test the timing functions overhead
 */

int main(int argc, char **argv)
{
    void *timer;

    timer = timing_new_timer(1); // true = nanoseconds precision

    timing_start(timer);
    int pid = getpid();
    printf("Time for getpid() syscall = %.2f, pid = %d\n", timing_end(timer), pid);

    timing_start(timer);
    int a = pid;
    printf("Time for doing interger assign = %.2f, a = %d\n", timing_end(timer), a);

    timing_start(timer);
    a = the_foo_function(pid);
    printf("Time for function call overhead = %.2f, a = %d\n", timing_end(timer), a);

}

int the_foo_function(int p)
{
    return (p-1);
}

#endif
