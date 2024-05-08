#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
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
#define MAX_THREADS 16


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
void *thread_test_bug(void *l);
int real_data_performance_test(void *lv);


char uafile[128];

int main(int argc, char **argv)
{
    char *str = NULL;
    int howmany = 1000;
    int j;
    int lru_err = 0;
    double check_time, add_time;
    unsigned long long delta;
    int nt = 16;

    if (argc != 4 )
    {
        printf("Usage : %s lrusize file_of_keys numthreads\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    howmany = atoi(argv[1]);
    strcpy(uafile, argv[2]);
    nt = atoi(argv[3]);

    if ( nt > 16 )
    {
        printf("Max %d threads please\n", MAX_THREADS);
        exit(EXIT_FAILURE);
    }

    char payloads[howmany][20];

    void *t = timing_new_timer(1);

    // for profiling
    //goto real_data_performance_test;

    for (int i = 0; i < howmany; i++)
    {
        sprintf(payloads[i], "%d", i);
    }

    lru_t *l = lru_create(howmany);

    int add_count = 0;

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
            add_time = compute_average(add_time, add_count, delta);
            add_count++;
        }
    }

    printf("Average lru_check time in nanosecs : %.2f\n", check_time);
    printf("Average lru_add time in nanosecs : %.2f\n", add_time);
    lru_clear(l);
    lru_destroy(l);

real_data_performance_test:

    printf("------------ STARTING KEY FILE PERFORMANCE tests --------- \n");
    l = lru_create(howmany);

    real_data_performance_test((void *) l);

    // goto end;

    lru_destroy(l);

    printf("------------ STARTING MULTI THREAD tests --------- \n");


    l = lru_create(howmany/10);

    for (int i = 0; i < howmany; i++)
    {
        char rnd[64];
        generate_random_str(rnd);
        sprintf(payloads[i], "%s-%d", rnd, i);
    }

    pthread_t tid[MAX_THREADS];

    for (int t=0; t<nt; t++)
    {
        // run threads working on the same fh
        pthread_create(&tid[t], NULL, &thread_test, (void *) l);
        printf("------------ thread %ld started --------- \n", tid[t]);
    }

    // wait for termination
    void *retval;
    for (int t=0; t<nt; t++)
    {
        pthread_t r = pthread_join(tid[t], &retval);
        printf("------------ thread %ld terminated --------- \n", r);
    }
    printf("------------ TERMINATING MULTI THREAD tests --------- \n");

    printf("------------ Reproducing MULTI THREAD Bug --------- \n");

    for (int t=0; t<nt; t++)
    {
        // run threads working on the same fh
        pthread_create(&tid[t], NULL, &thread_test_bug, (void *) l);
        printf("------------ thread %ld started --------- \n", tid[t]);
    }

    // wait for termination
    for (int t=0; t<nt; t++)
    {
        pthread_t r = pthread_join(tid[t], &retval);
        printf("------------ thread %ld terminated --------- \n", r);
    }

    printf("------------ END MULTI THREAD Bug --------- \n");

end:
    lru_clear(l);
    lru_destroy(l);

}

char payloads[500000][40];


void *thread_test(void *lv)
{
    int howmany = 100000;
    char *str = NULL;
    int j;
    int lru_err = 0;
    double check_time = 0.0;
    double add_time = 0.0;
    unsigned long long delta;
    int add_dup_errors = 0;
    int add_other_errors = 0;
    int add_err;

    lru_t *l = (lru_t *) lv;

    int tid = pthread_self();
    // srand(tid);

    void *t = timing_new_timer(1);

    int add_count = 0;

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
            add_time = compute_average(add_time, add_count, delta);
            add_count++;
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

#define MAX_UA (8*1024)

const char *static_payload = "payload to be retrieved from the lru_check function";

void *thread_test_bug(void *lv)
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
    char read_ua[MAX_UA];
    FILE *in;

    lru_t *l = (lru_t *) lv;

    int tid = pthread_self();

    if ((in = fopen(uafile, "rb")) == NULL)
    {
        perror(uafile);
        exit(EXIT_FAILURE);
    }

    int uacount = 0;

    while (fgets(read_ua, MAX_UA, in) != NULL)
    {
        j = rand_r(&tid) % howmany;

        add_err = lru_add(l, read_ua, (void *) static_payload);
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

        char *retrieved = NULL;

        lru_err = lru_check(l, read_ua, (void *) &retrieved);
        if (lru_err == LRU_OK)
        {
            if ( retrieved != static_payload)
            {
                printf("lru_check returns OK but pointers differ : retrieved = %p, static_payload = %p\n",
                       retrieved, static_payload);
            }
        }

    }

    printf("T %d add_dup_errors %d, add_other_errors %d\n", tid, add_dup_errors, add_other_errors);

}


int real_data_performance_test(void *lv)
{
    int howmany = 100000;
    int j;
    int lru_err = 0;
    double check_time, add_time;
    unsigned long long delta;
    int add_dup_errors = 0;
    int add_other_errors = 0;
    int add_err;
    char read_ua[MAX_UA];
    FILE *in;
    char str[64];
    char *generic = "generic";

    lru_t *l = (lru_t *) lv;
    void *t = timing_new_timer(1);

    if ((in = fopen(uafile, "rb")) == NULL)
    {
        perror(uafile);
        exit(EXIT_FAILURE);
    }

    int uacount = 0;
    int addcount = 0;

    while (fgets(read_ua, MAX_UA, in) != NULL)
    {
        timing_start(t);
        lru_err = lru_check(l, read_ua, (void *) &str);
        delta = timing_end(t);

        check_time = compute_average(check_time, uacount, delta);

        if (lru_err == LRU_ELEMENT_NOT_FOUND)
        {
            // this ua not in cache, add it
            timing_start(t);
            add_err = lru_add(l, read_ua, (void *) generic);
            delta = timing_end(t);
            add_time = compute_average(add_time, addcount, delta);
            addcount++;
        }

        char *retrieved = NULL;
        uacount++;
    }

    printf("Average lru_check time in nanosecs : %.2f\n", check_time);
    printf("Average lru_add time in nanosecs : %.2f\n", add_time);

}
