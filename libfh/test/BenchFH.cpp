#include "picobench/picobench.hpp"

#include <vector>

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "fh.h"

static void generate_random_str(int seed, char *str, int min_len, int max_len);
unsigned int murmur64a_hash(char *key, int dim);


// Benchmarking libfh fh_get() with standard hasfunction - baseline
static void BenchFHSmallGet(picobench::state &s)
{
    fh_t *fh = NULL;
    int err = 0;
    char key[65];

    // Create hash table of strings with default hash function
    // oat hash (one at a time hash), Bob Jenkins, used by cfu hash and perl
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

// Benchmarking libfh fh_get() with standard hasfunction - baseline
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


// murmur64a_hash algorithm

#define BIG_CONSTANT(x) (x ## LLU)

unsigned int murmur64a_hash(char *key, int dim)
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

    return h & (dim - 1);
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
