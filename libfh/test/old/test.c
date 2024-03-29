#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>


#include    "fh.h"
#include    "timing.h"


#define HASH_SIZE 60000

// old hash function
static unsigned int fh_default_hash_orig(char *name, int i)
{
    unsigned long h = 0, g;
    while (*name)
    {
        h = (h << 4) + *name++;
        g = (h & 0xF0000000);
        if (g)
            h ^= g >> 24;
        h &= ~g;
    }
    return h % i;
}

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

void *string_thread_test(void *f);

int size = HASH_SIZE;

int main( int argc, char **argv )
{
    fh_t *f;
    char mykey[64];
    struct mydata
    {
        char buffer[64];
        char checksum[128];
        int i;
    } md, md2;
    double insert_time;
    double search_time;
    unsigned long long delta;
    void *t = timing_new_timer(1);

    if ( argc > 1 )
    {
        size = atoi(argv[1]);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// fixed size opaque data
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    printf("------------ Testing fixed size opaque data hashtable \n");

    f = fh_create(size, sizeof(struct mydata), NULL);

    if ( f == NULL )
    {
        printf("fh_create returned NULL\n");
    }

    int real_size = 0;

    if ( !fh_getattr(f, FH_ATTR_DIM, &real_size) )
    {
        printf("error in fh_getattr\n");
    }
    printf("hash real size %d\n", real_size);

    for (int i = 0; i< size/2; i++ )
    {
        sprintf(md.buffer, "%d", i);
        md.i = i;

        timing_start(t);
        int err = fh_insert(f, md.buffer, &md);
        delta = timing_end(t);
        insert_time = compute_average(insert_time, i, delta);

        if ( err < 0 )
        {
            printf("error %d in fh_insert\n", err);
        }
    }
    printf("Average insert time in nanosecs : %.2f\n", insert_time);

    int curr;
    if ( !fh_getattr(f, FH_ATTR_ELEMENT, &curr) )
    {
        printf("error in fh_getattr\n");
    }
    printf("hash elements %d\n", curr);

    int coll;
    if ( !fh_getattr(f, FH_ATTR_COLLISION, &coll) )
    {
        printf("error in fh_getattr\n");
    }
    printf("hash collision %d\n", coll);

    // search
    printf("searching ..\n");
    for (int j = 1; j <10; j++)
    {
        for (int i = 0; i< size/2; i++ )
        {
            sprintf(md.buffer, "%d", i);
            struct mydata *m;

            timing_start(t);
            int err = fh_search(f, md.buffer, &md, sizeof(md));
            delta = timing_end(t);
            search_time = compute_average(search_time, i*j, delta);

            if ( err < 0 )
            {
                printf("error %d in fh_search\n", err);
            }
            if ( md.i != i)
            {
                printf("error in search md.%d != %d\n", md.i, i);
            }

            // use also fh_get
            m = fh_get(f, md.buffer, &err);
            if ( m == NULL)
            {
                printf("error %d in fh_get\n", err);
            }
            if ( m->i != i)
            {
                printf("error in fh_get m->%d != %d\n", m->i, i);
            }
        }
    }

    printf("Average access time in nanosecs : %.2f\n", search_time);

    printf("deleting ..\n");
    for (int i = 0; i< size/2; i++ )
    {
        sprintf(md.buffer, "%d", i);
        int err = fh_del(f, md.buffer);
        if ( err < 0 )
        {
            printf("error %d in fh_del\n", err);
        }
    }

    if ( !fh_getattr(f, FH_ATTR_ELEMENT, &curr) )
    {
        printf("error in fh_getattr\n");
    }
    printf("hash elements %d\n", curr);

    // scan start on empty hash should give error
    void *slot;
    int scanerr = fh_scan_start(f, 0, &slot);
    if ( scanerr != FH_ELEMENT_NOT_FOUND)
    {
        // error
        printf("fh_scan_start on an empty hash should give error");
    }

    // fill it
    for (int i = 0; i< size/10; i++ )
    {
        sprintf(md.buffer, "%d", i);
        md.i = i;

        int err = fh_insert(f, md.buffer, &md);
        if ( err < 0 )
        {
            printf("error %d in fh_insert\n", err);
        }
    }

    // now scan
    int index = fh_scan_start(f, 0, &slot);

    while (fh_scan_next(f, &index, &slot, mykey, &md2, sizeof(md2)) != FH_SCAN_END )
    {
        //printf("mykey %s, md2.i = %d\n", mykey, md2.i);
        if ( strcmp(mykey, md2.buffer) != 0 )
        {
            printf("error in fh_scan_next\n");
        }
    }

    fh_destroy(f);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// string opaque data
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    printf("------------ Testing string opaque data \n");
    if ((f = fh_create(size, FH_DATALEN_STRING, NULL)) == NULL )
    {
        printf("fh_create returned NULL\n");
    }

    real_size = 0;

    if ( !fh_getattr(f, FH_ATTR_DIM, &real_size) )
    {
        printf("error in fh_getattr\n");
    }
    printf("hash real size %d\n", real_size);

    insert_time = 0;
    for (int i = 0; i< size/2; i++ )
    {
        sprintf(md.buffer, "%06d", i);
        sprintf(md.checksum, "%0x", i);
        // printf("generating checksum %s\n", md.checksum);
        md.i = i;

        timing_start(t);
        int err = fh_insert(f, md.buffer, md.checksum);
        delta = timing_end(t);
        insert_time = compute_average(insert_time, i, delta);

        if ( err < 0 )
        {
            printf("error %d in fh_insert\n", err);
        }
    }
    printf("Average insert time in nanosecs : %.2f\n", insert_time);

    if ( !fh_getattr(f, FH_ATTR_ELEMENT, &curr) )
    {
        printf("error in fh_getattr\n");
    }
    printf("hash elements %d\n", curr);

    coll = 0;
    if ( !fh_getattr(f, FH_ATTR_COLLISION, &coll) )
    {
        printf("error in fh_getattr\n");
    }
    printf("hash collision %d\n", coll);

// search
    printf("searching ..\n");
    search_time = 0;
    for (int j = 0; j <10; j++)
    {
        for (int i = 0; i< size/2; i++ )
        {
            char cksum[128];
            char *csum;
            sprintf(md.buffer, "%06d", i);
            sprintf(cksum, "%0x", i);

            timing_start(t);
            int err = fh_search(f, md.buffer, md.checksum, 128);
            delta = timing_end(t);
            search_time = compute_average(search_time, i*j, delta);

            if ( err < 0 )
            {
                printf("error %d in fh_search\n", err);
            }
            if ( strcmp(cksum, md.checksum) != 0 )
            {
                printf("error in search md.checksum %s != %d checksum %s\n", md.checksum, i, cksum);
            }

            csum = fh_get(f, md.buffer, &err);
            if (csum == NULL)
            {
                printf("error %d in fh_get\n", err);
            }

            if ( strcmp(csum, cksum) != 0 )
            {
                printf("error in fh_get : ret %s != %d checksum %s\n", csum, i, cksum);
            }
        }
    }
    printf("Average access time in nanosecs : %.2f\n", search_time);

    printf("deleting ..\n");
    for (int i = 0; i< size/2; i++ )
    {
        sprintf(md.buffer, "%06d", i);
        int err = fh_del(f, md.buffer);
        if ( err < 0 )
        {
            printf("error %d in fh_del\n", err);
        }
    }

    if ( !fh_getattr(f, FH_ATTR_ELEMENT, &curr) )
    {
        printf("error in fh_getattr\n");
    }
    printf("hash elements %d\n", curr);

// scan start on empty hash should give error

    scanerr = fh_scan_start(f, 0, &slot);
    if ( scanerr != FH_ELEMENT_NOT_FOUND)
    {
        // error
        printf("fh_scan_start on an empty hash should give error");
    }

// fill it
    for (int i = 0; i< size/2; i++ )
    {
        sprintf(md.buffer, "%06d", i);
        sprintf(md.checksum, "%0x", i);
        //printf("generating checksum %s\n", md.checksum);
        md.i = i;

        int err = fh_insert(f, md.buffer, md.checksum);
        if ( err < 0 )
        {
            printf("error %d in fh_insert\n", err);
        }
    }

// now scan
    index = fh_scan_start(f, 0, &slot);

    while (fh_scan_next(f, &index, &slot, mykey, md2.checksum, 128) != FH_SCAN_END )
    {
        //printf("mykey %s, md2.i = %d\n", mykey, md2.i);
        char cksum[128];
        sprintf(cksum, "%0x", atoi(mykey));
        if ( strcmp(cksum, md2.checksum) != 0 )
        {
            printf("error in fh_scan_next\n");
        }
    }
    fh_destroy(f);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// pointer opaque data
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    printf("------------ Testing pointer opaque data \n");
    if ((f = fh_create(size, FH_DATALEN_VOIDP, NULL)) == NULL )
    {
        printf("fh_create returned NULL\n");
    }
    real_size = 0;
    if ( !fh_getattr(f, FH_ATTR_DIM, &real_size) )
    {
        printf("error in fh_getattr\n");
    }
    printf("hash real size %d\n", real_size);

    insert_time = 0;
    char key[64];
    for (int i = 0; i< size/2; i++ )
    {
        sprintf(key, "%06d", i);
        char *checksum = malloc(64);
        sprintf(checksum, "%0x", i);
        // printf("generating checksum %s\n", md.checksum);

        timing_start(t);
        int err = fh_insert(f, key, checksum);
        delta = timing_end(t);
        insert_time = compute_average(insert_time, i, delta);

        if ( err < 0 )
        {
            printf("error %d in fh_insert\n", err);
        }
    }
    printf("Average insert time in nanosecs : %.2f\n", insert_time);

    if ( !fh_getattr(f, FH_ATTR_ELEMENT, &curr) )
    {
        printf("error in fh_getattr\n");
    }
    printf("hash elements %d\n", curr);

    coll = 0;
    if ( !fh_getattr(f, FH_ATTR_COLLISION, &coll) )
    {
        printf("error in fh_getattr\n");
    }
    printf("hash collision %d\n", coll);

// search
    printf("searching ..\n");
    search_time = 0;

    for (int j = 0; j <10; j++)
    {
        for (int i = 0; i< size/2; i++ )
        {
            char cksum[128];
            char *csum;
            int err;
            sprintf(key, "%06d", i);
            sprintf(cksum, "%0x", i);

            timing_start(t);
            csum = fh_get(f, key, &err);
            delta = timing_end(t);
            search_time = compute_average(search_time, i*j, delta);

            if (csum == NULL)
            {
                printf("error %d in fh_get\n", err);
            }

            if ( strcmp(csum, cksum) != 0 )
            {
                printf("error in fh_get : ret %s != %d checksum %s\n", csum, i, cksum);
            }
        }
    }
    printf("Average access time in nanosecs : %.2f\n", search_time);

    printf("deleting ..\n");
    for (int i = 0; i< size/2; i++ )
    {
        char *csum;
        int err;

        sprintf(md.buffer, "%06d", i);
        csum = fh_get(f, md.buffer, &err);
        if (csum == NULL)
        {
            printf("error %d in fh_get\n", err);
        }
        else
        {
            free(csum);
        }

        err = fh_del(f, md.buffer);
        if ( err < 0 )
        {
            printf("error %d in fh_del\n", err);
        }
    }

    if ( !fh_getattr(f, FH_ATTR_ELEMENT, &curr) )
    {
        printf("error in fh_getattr\n");
    }
    printf("hash elements %d\n", curr);


    fh_destroy(f);
    printf("------------ end of tests \n");


    /////////////////////////////
    /// multi-thread tests
    /////////////////////////////

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
}

/*
 * multithreaded test
 */

void *string_thread_test(void *fv)
{
    char mykey[64];
    struct mydata
    {
        char buffer[64];
        char checksum[128];
        int i;
    } md, md2;
    double insert_time;
    double search_time;
    unsigned long long delta;
    void *t = timing_new_timer(1);
    int curr, coll;
    void *slot;
    fh_t *f = (fh_t *) fv;

    int tid = pthread_self();
    srand(tid);

    printf("------------ %d - Testing string opaque data \n", tid);

    int real_size = 0;


    insert_time = 0;
    for (int i = 0; i< size/2; i++ )
    {
        sprintf(md.buffer, "%06d", i);
        sprintf(md.checksum, "%0x", i);
        // printf("generating checksum %s\n", md.checksum);
        md.i = i;

        timing_start(t);
        int err = fh_insert(f, md.buffer, md.checksum);
        delta = timing_end(t);
        insert_time = compute_average(insert_time, i, delta);

        if ( err < 0 && err != FH_DUPLICATED_ELEMENT)
        {
            printf("error %d in fh_insert\n", err);
        }
    }
    printf("Average insert time in nanosecs : %.2f\n", insert_time);

    if ( !fh_getattr(f, FH_ATTR_ELEMENT, &curr) )
    {
        printf("error in fh_getattr\n");
    }
    printf("hash elements %d\n", curr);

    coll = 0;
    if ( !fh_getattr(f, FH_ATTR_COLLISION, &coll) )
    {
        printf("error in fh_getattr\n");
    }
    printf("hash collision %d\n", coll);

// search
    printf("searching ..\n");
    search_time = 0;
    for (int j = 0; j <10; j++)
    {
        for (int i = 0; i< size/2; i++ )
        {
            char cksum[128];
            char *csum;

            int ii = rand() % (size/2);
            sprintf(md.buffer, "%06d", ii);
            sprintf(cksum, "%0x", ii);

            timing_start(t);
            int err = fh_search(f, md.buffer, md.checksum, 128);
            delta = timing_end(t);
            search_time = compute_average(search_time, i*j, delta);

            if (err < 0 && err != FH_ELEMENT_NOT_FOUND)
            {
                printf("error %d in fh_search\n", err);
                continue;
            }

            if ( err < 0 ) continue;

            if ( strcmp(cksum, md.checksum) != 0 )
            {
                printf("thread %d, error in search md.checksum %s != %d checksum %s (fh_search ret %d)\n",
                       tid, md.checksum, ii, cksum, err);
                return(0);
            }
        }
    }

    printf("Average access time in nanosecs : %.2f\n", search_time);

    printf("deleting ..\n");
    for (int i = 0; i< size/2; i++ )
    {
        sprintf(md.buffer, "%06d", i);
        int err = fh_del(f, md.buffer);
        if ( err < 0 && err != FH_ELEMENT_NOT_FOUND)
        {
            printf("error %d in fh_del\n", err);
        }
    }

    if ( !fh_getattr(f, FH_ATTR_ELEMENT, &curr) )
    {
        printf("error in fh_getattr\n");
    }
    printf("hash elements %d\n", curr);
}
