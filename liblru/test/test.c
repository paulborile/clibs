#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>


#include    "lru.h"
#include    "timing.h"


double  compute_average(double current_avg, int count, int new_value)
{
    if ( count == 0 )
    {
        return (new_value);
    }
    else
    {
        return (((current_avg * count) + new_value) / (count+1));
    }
}

int main(int argc, char **argv)
{
    int howmany = atoi(argv[1]);
    char *str = NULL;
    char payloads[howmany][20];
    int j;
    int lru_err = 0;
    double check_time, add_time;
    unsigned long long delta;

    void *t = timing_new_timer(1);

    for (int i = 0; i < howmany; i++)
    {
        sprintf(payloads[i], "%d", i);
    }

    lru_t *l = lru_create(howmany);

    for (int i = 0; i < howmany*1000; i++)
    {
        j = rand() % howmany;

        timing_start(t);
        lru_err = lru_check(l, payloads[j], (void *) &str);
        delta = timing_end(t);
        check_time = compute_average(check_time, i, delta);


        if ( lru_err == LRU_OK)
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
            timing_start(t);
            lru_add(l, payloads[j], (void *) payloads[j]);
            delta = timing_end(t);
            add_time = compute_average(add_time, i, delta);

        }
    }

    printf("Average lru_check time in nanosecs : %.2f\n", check_time);
    printf("Average lru_add time in nanosecs : %.2f\n", add_time);

/*
   printf("------------ STARTING MULTI THREAD tests --------- \n");


   if ((f = fh_create(size, FH_DATALEN_STRING, NULL)) == NULL )
   {
    printf("fh_create returned NULL\n");
   }

   #define MAX_THREADS 16

   pthread_t tid[MAX_THREADS];


   for (int t=0; t<MAX_THREADS; t++)
   {
    // run threads working on the same fh
    pthread_create(&tid[t], NULL, &string_thread_test, (void *) f);
    printf("------------ thread %ld started --------- \n", tid[t]);
   }

   // wait for termination
   void *retval;
   for (int t=0; t<MAX_THREADS; t++)
   {
    pthread_t r = pthread_join(tid[t], &retval);
    printf("------------ thread %ld terminated --------- \n", r);
   }
   printf("------------ TERMINATING MULTI THREAD tests --------- \n");
 */

}
