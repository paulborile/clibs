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

#define MAX_STR  30
#define MIN_STR  10
#define ASCII0      'a'
#define ASCIISET    'z'


void generate_random_str(char *str)
{
    int i, irandom;
    char c;

    irandom = MIN_STR + (rand() % (MAX_STR - MIN_STR));

    for (i = 0; i < irandom; i++)
    {
        c = ASCII0 + (rand() % (ASCIISET - ASCII0 + 1));
        str[i] = c;
    }

    str[i] = '\0';

    return;
}


void *thread_test(void *l);

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
    lru_destroy(l);

    printf("------------ STARTING MULTI THREAD tests --------- \n");



    l = lru_create(howmany/10);

    for (int i = 0; i < howmany; i++)
    {
        generate_random_str(payloads[i]);
        sprintf(payloads[i], "%s-%d", payloads[i], i);
    }


#define MAX_THREADS 16

    pthread_t tid[MAX_THREADS];


    for (int t=0; t<MAX_THREADS; t++)
    {
        // run threads working on the same fh
        pthread_create(&tid[t], NULL, &thread_test, (void *) l);
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

}

char payloads[500000][40];


void *thread_test(void *lv)
{
    int howmany = 100000;
    char *str = NULL;
    int j;
    int lru_err = 0;
    double check_time, add_time;
    unsigned long long delta;
    int add_dup_errors = 0;
    int add_other_errors = 0;
    int add_err;

    lru_t *l = (lru_t *) lv;

    int tid = pthread_self();
    // srand(tid);

    void *t = timing_new_timer(1);

    for (int i = 0; i < howmany*10; i++)
    {
        j = rand_r(&tid) % howmany;

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
            add_err = lru_add(l, payloads[j], (void *) payloads[j]);
            delta = timing_end(t);
            add_time = compute_average(add_time, i, delta);
            if (add_err != LRU_OK)
            {
                if ( add_err == LRU_DUPLICATED_ELEMENT )
                {
                    add_dup_errors++;
                }
                else
                {
                    add_other_errors++;
                }
            }
        }
    }

    printf("T %d Average lru_check time in nanosecs : %.2f\n", tid, check_time);
    printf("T %d Average lru_add time in nanosecs : %.2f, add_dup_errors %d, add_other_errors %d\n", tid, add_time, add_dup_errors, add_other_errors);

}
