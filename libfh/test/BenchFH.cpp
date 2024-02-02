#include "picobench/picobench.hpp"

#include <vector>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "fh.h"

static void generate_random_str(int seed, char *str, int min_len, int max_len);
uint64_t    murmur64a_hash(void *data, char *key);
/*
 * oat hash (one at a time hash), Bob Jenkins, used by cfu hash and perl
 */
uint64_t fh_default_hash(void *data, char *key);


PICOBENCH_SUITE("BenchFH");
// Benchmarking libfh fh_get() with standard hasfunction - baseline
static void BenchFHSmallGet(picobench::state &s)
{
    fh_t *fh = NULL;
    int err = 0;
    char key[65];

    // Create hash table of strings with default hash function
    fh = fh_create(200, FH_DATALEN_STRING, NULL);

    for (int i = 0; i<200; i++ )
    {
        generate_random_str(i, key, 32, 64);
        fh_insert(fh, key, (void *)"payload");
    }
    int size = 0;
    // fh_getattr(fh, FH_ATTR_ELEMENT, &size);
    // printf("Inserted %d elements, collisions %d\n", size, fh->h_collision);

    for (auto _ : s)
    {
        fh_get(fh, (char *) "flexpeqorgyjtdszopgvchbcfqmghgujbypiufdaprkpxixejr", &err);
    }
    fh_destroy(fh);

}
// Register the above function with picobench
PICOBENCH(BenchFHSmallGet).label("BenchFHSmallGet").samples(10).iterations({10000, 100000, 1000000});

// Benchmarking libfh fh_get() with murmur64a_hash - baseline
static void BenchFHSmallGetMurmurHash(picobench::state &s)
{
    fh_t *fh = NULL;
    int err = 0;
    char key[65];

    // Create hash table of strings with murmur64a hash
    fh = fh_create(200, FH_DATALEN_STRING, murmur64a_hash);

    for (int i = 0; i<200; i++ )
    {
        generate_random_str(i, key, 32, 64);
        fh_insert(fh, key, (void *)"payload");
    }
    int size = 0;
    // fh_getattr(fh, FH_ATTR_ELEMENT, &size);
    // printf("Inserted %d elements, collisions %d\n", size, fh->h_collision);

    for (auto _ : s)
    {
        fh_get(fh, (char *) "flexpeqorgyjtdszopgvchbcfqmghgujbypiufdaprkpxixejr", &err);
    }
    fh_destroy(fh);

}
// Register the above function with picobench
PICOBENCH(BenchFHSmallGetMurmurHash).label("BenchFHSmallGetMurmurHash").samples(10).iterations({10000, 100000, 1000000});

PICOBENCH_SUITE("HashFunctions");
// Benchmarking standard hash function - baseline
static void HashFunctionsDefaultX100(picobench::state &s)
{
    unsigned int hash;
    char key[65];
    int i = 0;
    fh_t fh;

    generate_random_str(i, key, 32, 64);

    for (auto _ : s)
    {
        for (i = 0; i<100; i++)
        {
            hash += fh_default_hash(&fh, key) & 127;
        }
    }
    // just to avoid optimizer removing all code..
    if (hash == 1)
    {
        printf("%d\n", hash);
    }
}
PICOBENCH(HashFunctionsDefaultX100).label("HashFunctionsDefaultX100").samples(10).iterations({1000, 10000, 100000});

// Benchmarking murmurhash hash function - baseline
static void HashFunctionsMurmurX100(picobench::state &s)
{
    unsigned int hash;
    char key[65];
    int i = 0;
    fh_t fh;

    generate_random_str(i, key, 32, 64);

    for (auto _ : s)
    {
        for (i = 0; i<100; i++)
        {
            hash += murmur64a_hash(&fh, key) & 127;
        }
    }
    // just to avoid optimizer removing all code..
    if (hash == 1)
    {
        printf("%d\n", hash);
    }
}
PICOBENCH(HashFunctionsMurmurX100).label("HashFunctionsMurmurX100").samples(10).iterations({1000, 10000, 100000});

PICOBENCH_SUITE("GenerateRandomString");
static void GenerateRandomString(picobench::state &s)
{
    char key[65];
    int i = 0;

    for (auto _ : s)
    {
        generate_random_str(i++, key, 32, 64);
    }
    // just to avoid optimizer removing all code..
    if (key[0] == 1)
    {
        printf("%s\n", key);
    }
}
PICOBENCH(GenerateRandomString).label("GenerateRandomString").samples(10);

// murmur64a_hash algorithm

#define BIG_CONSTANT(x) (x ## LLU)

uint64_t    murmur64a_hash(void *unused, char *key)
{
    const uint64_t m = BIG_CONSTANT(0xc6a4a7935bd1e995);
    const int r = 47;
    int len = strlen(key);
    uint64_t h = 0x1F0D3804 ^ (len * m);

    const uint64_t *data = (const uint64_t *)key;
    const uint64_t *end = data + (len/8);

    while (data != end)
    {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const unsigned char *data2 = (const unsigned char *)data;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough="

    switch (len & 7)
    {
    case 7:
        h ^= (uint64_t) data2[6] << 48;
    case 6:
        h ^= (uint64_t) data2[5] << 40;
    case 5:
        h ^= (uint64_t) data2[4] << 32;
    case 4:
        h ^= (uint64_t) data2[3] << 24;
    case 3:
        h ^= (uint64_t) data2[2] << 16;
    case 2:
        h ^= (uint64_t) data2[1] << 8;
    case 1:
        h ^= (uint64_t) data2[0];
        h *= m;
    };

#pragma GCC diagnostic pop

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}


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


PICOBENCH_SUITE("BigHash");
// Benchmarking libfh fh_get() with standard hasfunction - baseline
static void BenchFHBigHashGet(picobench::state &s)
{
    fh_t *fh = NULL;
    int err = 0;
    char key[65];
    int num_strings = 1000000; // simulating 1 million random keys
    char *keys[num_strings];


    // Create hash table of strings with default hash function
    fh = fh_create(num_strings, FH_DATALEN_STRING, NULL);

    // populate string array and insert in hash

    printf("starting load of hash 1m entries\n");

    for (int i = 0; i<num_strings; i++ )
    {
        generate_random_str(i, key, 32, 64);
        keys[i] = strdup(key);
        fh_insert(fh, keys[i], (void *)"payload");
    }

    printf("starting benchmark\n");

    int j = 0;

    for (auto _ : s)
    {
        if ( j == num_strings )
            j == 0;
        fh_get(fh, keys[j++], &err);
    }

    printf("starting cleanup\n");

    fh_destroy(fh);
    for (int i = 0; i<num_strings; i++ )
    {
        free(keys[i]);
    }

}
// Register the above function with picobench
PICOBENCH(BenchFHBigHashGet).label("BenchFHBigHashGet").iterations({1000, 10000, 100000});

// Benchmarking libfh fh_get() with murmur hash
static void BenchFHBigHashGetMurmur(picobench::state &s)
{
    fh_t *fh = NULL;
    int err = 0;
    char key[65];
    int num_strings = 1000000; // simulating 1 million random keys
    char *keys[num_strings];


    // Create hash table of strings with default hash function
    fh = fh_create(num_strings, FH_DATALEN_STRING, murmur64a_hash);

    // populate string array and insert in hash

    printf("starting load of hash 1m entries\n");

    for (int i = 0; i<num_strings; i++ )
    {
        generate_random_str(i, key, 32, 64);
        keys[i] = strdup(key);
        fh_insert(fh, keys[i], (void *)"payload");
    }

    printf("starting benchmark\n");

    int j = 0;

    for (auto _ : s)
    {
        if ( j == num_strings )
            j == 0;
        fh_get(fh, keys[j++], &err);
    }

    printf("starting cleanup\n");

    fh_destroy(fh);
    for (int i = 0; i<num_strings; i++ )
    {
        free(keys[i]);
    }

}
// Register the above function with picobench
PICOBENCH(BenchFHBigHashGetMurmur).label("BenchFHBigHashGetMurmur").iterations({1000, 10000, 100000});
