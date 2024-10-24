#include "picobench/picobench.hpp"

#include <vector>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "avl.h"

static void generate_random_str(int seed, char *str, int min_len, int max_len);
PICOBENCH_SUITE("BenchAVL");
// Benchmarking libavl avl_search() with standard hasfunction - baseline
static void BenchAVLSmallGetWorstCase(picobench::state &s)
{
    avl_h *avl = NULL;
    int err = 0;
    char key[65];

    // Create avl tree of strings
    avl = avl_create(NULL);

    for (int i = 0; i<200; i++ )
    {
        generate_random_str(i, key, 32, 64);
        avl_insert(avl, key, (void *)"payload");
    }
    int size = 0;
    // avl_getattr(avl, AVL_ATTR_ELEMENT, &size);
    // printf("Inserted %d elements, collisions %d\n", size, avl->h_collision);

    for (auto _ : s)
    {
        avl_search(avl, (char *) "flexpeqorgyjtdszopgvchbcfqmghgujbypiufdaprkpxixejr");
    }
    avl_destroy(avl);

}
// Register the above function with picobench
PICOBENCH(BenchAVLSmallGetWorstCase).label("BenchAVLSmallGet").samples(10).iterations({10000, 100000, 1000000});


PICOBENCH_SUITE("BigAVL");
// Benchmarking libavl avl_search() with standard hasfunction - baseline
static void BenchAVLBigGet(picobench::state &s)
{
    avl_h *avl = NULL;
    int err = 0;
    char key[65];
    int num_strings = 1000000; // simulating 1 million random keys
    char *keys[num_strings];


    // Create avl tree of strings
    avl = avl_create(NULL);

    // populate string array and insert in avl

    printf("starting load of avl tree 1m entries\n");

    for (int i = 0; i<num_strings; i++ )
    {
        generate_random_str(i, key, 32, 64);
        keys[i] = strdup(key);
        avl_insert(avl, keys[i], (void *)"payload");
    }

    printf("starting benchmark\n");

    int j = 0;

    for (auto _ : s)
    {
        if ( j == num_strings )
        {
            j == 0;
        }
        avl_search(avl, keys[j++]);
    }

    printf("starting cleanup\n");

    avl_destroy(avl);
    for (int i = 0; i<num_strings; i++ )
    {
        free(keys[i]);
    }

}
// Register the above function with picobench
PICOBENCH(BenchAVLBigGet).label("BenchAVLBigGet").iterations({1000, 10000, 100000, 1000000});



#define ASCII0      'a'
#define ASCIISET    'z'
static void generate_random_str(int seed, char *str, int min_len, int max_len)
{
    int i, irandom;
    char c;
    unsigned int s = seed;
    irandom = min_len + (rand_r(&s) % (max_len - min_len));
    for (i = 0; i < irandom; i++)
    {
        c = ASCII0 + (rand_r(&s) % (ASCIISET - ASCII0 + 1));
        str[i] = c;
    }
    str[i] = '\0';
    // printf("%s\n", str);
    return;
}
