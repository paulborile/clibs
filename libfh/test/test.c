#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>


#include    "fh.h"
#include    "timing.h"


#define HASH_SIZE 300000

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

int main( int argc, char **argv )
{
    fh_t *f;
    int size = HASH_SIZE;
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

    printf("Testing fixed size opaque data\n");

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

    for (int i = 0; i< HASH_SIZE/2; i++ )
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

    for (int i = 0; i< HASH_SIZE/2; i++ )
    {
        sprintf(md.buffer, "%d", i);
        struct mydata *m;

        timing_start(t);
        int err = fh_search(f, md.buffer, &md, sizeof(md));
        delta = timing_end(t);
        search_time = compute_average(search_time, i, delta);

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

    printf("Average access time in nanosecs : %.2f\n", search_time);

    printf("deleting ..\n");
    for (int i = 0; i< HASH_SIZE/2; i++ )
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
    for (int i = 0; i< HASH_SIZE/10; i++ )
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
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    printf("Testing string opaque data --------------\n");


    f = fh_create(size, -1, NULL);

    if ( f == NULL )
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
    for (int i = 0; i< HASH_SIZE/2; i++ )
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
    for (int i = 0; i< HASH_SIZE/2; i++ )
    {
        char cksum[128];
        char *csum;
        sprintf(md.buffer, "%06d", i);
        sprintf(cksum, "%0x", i);

        timing_start(t);
        int err = fh_search(f, md.buffer, md.checksum, 128);
        delta = timing_end(t);
        search_time = compute_average(search_time, i, delta);

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
    printf("Average access time in nanosecs : %.2f\n", search_time);

    printf("deleting ..\n");
    for (int i = 0; i< HASH_SIZE/2; i++ )
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
    for (int i = 0; i< HASH_SIZE/2; i++ )
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

}
