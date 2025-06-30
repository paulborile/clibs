#include <benchmark/benchmark.h>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <pthread.h>

#include "fh.h"

// Forward declarations
static void generate_random_str(int seed, char *str, int min_len, int max_len);
uint64_t murmur64a_hash(void *data, char *key);
uint64_t fh_default_hash(void *data, char *key);

// --- BenchFHSmallGet ---
static void BenchFHSmallGet(benchmark::State& state)
{
    fh_t *fh = fh_create(200, FH_DATALEN_STRING, NULL);
    char key[65];
    int err = 0;

    for (int i = 0; i < 200; ++i) {
        generate_random_str(i, key, 32, 64);
        fh_insert(fh, key, (void *)"payload");
    }

    for (auto _ : state) {
        fh_get(fh, (char *)"flexpeqorgyjtdszopgvchbcfqmghgujbypiufdaprkpxixejr", &err);
    }
    fh_destroy(fh);
}
BENCHMARK(BenchFHSmallGet)->Iterations(10000)->Iterations(100000)->Iterations(1000000);

// --- BenchFHSmallGetMurmurHash ---
static void BenchFHSmallGetMurmurHash(benchmark::State& state)
{
    fh_t *fh = fh_create(200, FH_DATALEN_STRING, murmur64a_hash);
    char key[65];
    int err = 0;

    for (int i = 0; i < 200; ++i) {
        generate_random_str(i, key, 32, 64);
        fh_insert(fh, key, (void *)"payload");
    }

    for (auto _ : state) {
        fh_get(fh, (char *)"flexpeqorgyjtdszopgvchbcfqmghgujbypiufdaprkpxixejr", &err);
    }
    fh_destroy(fh);
}
BENCHMARK(BenchFHSmallGetMurmurHash)->Iterations(10000)->Iterations(100000)->Iterations(1000000);

// --- HashFunctionsDefaultX100 ---
static void HashFunctionsDefaultX100(benchmark::State& state)
{
    unsigned int hash = 0;
    char key[65];
    int i = 0;
    fh_t fh;
    generate_random_str(i, key, 32, 64);

    for (auto _ : state) {
        for (i = 0; i < 100; ++i) {
            hash += fh_default_hash(&fh, key) & 127;
        }
    }
    benchmark::DoNotOptimize(hash);
}
BENCHMARK(HashFunctionsDefaultX100)->Iterations(1000)->Iterations(10000)->Iterations(100000);

// --- HashFunctionsMurmurX100 ---
static void HashFunctionsMurmurX100(benchmark::State& state)
{
    unsigned int hash = 0;
    char key[65];
    int i = 0;
    fh_t fh;
    generate_random_str(i, key, 32, 64);

    for (auto _ : state) {
        for (i = 0; i < 100; ++i) {
            hash += murmur64a_hash(&fh, key) & 127;
        }
    }
    benchmark::DoNotOptimize(hash);
}
BENCHMARK(HashFunctionsMurmurX100)->Iterations(1000)->Iterations(10000)->Iterations(100000);

// --- GenerateRandomString ---
static void GenerateRandomString(benchmark::State& state)
{
    char key[65];
    int i = 0;
    for (auto _ : state) {
        generate_random_str(i++, key, 32, 64);
    }
    benchmark::DoNotOptimize(key);
}
BENCHMARK(GenerateRandomString);

// --- murmur64a_hash implementation ---
#define BIG_CONSTANT(x) (x ## LLU)
uint64_t murmur64a_hash(void *unused, char *key)
{
    const uint64_t m = BIG_CONSTANT(0xc6a4a7935bd1e995);
    const int r = 47;
    int len = strlen(key);
    uint64_t h = 0x1F0D3804 ^ (len * m);

    const uint64_t *data = (const uint64_t *)key;
    const uint64_t *end = data + (len / 8);

    while (data != end) {
        uint64_t k = *data++;
        k *= m;
        k ^= k >> r;
        k *= m;
        h ^= k;
        h *= m;
    }

    const unsigned char *data2 = (const unsigned char *)data;
    switch (len & 7) {
    case 7: h ^= (uint64_t)data2[6] << 48;
    case 6: h ^= (uint64_t)data2[5] << 40;
    case 5: h ^= (uint64_t)data2[4] << 32;
    case 4: h ^= (uint64_t)data2[3] << 24;
    case 3: h ^= (uint64_t)data2[2] << 16;
    case 2: h ^= (uint64_t)data2[1] << 8;
    case 1: h ^= (uint64_t)data2[0]; h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;
    return h;
}

// --- generate_random_str implementation ---
#define ASCII0   'a'
#define ASCIISET 'z'
static void generate_random_str(int seed, char *str, int min_len, int max_len)
{
    int i, irandom;
    char c;
    unsigned int s = seed;
    irandom = min_len + (rand_r(&s) % (max_len - min_len));
    for (i = 0; i < irandom; i++) {
        c = ASCII0 + (rand_r(&s) % (ASCIISET - ASCII0 + 1));
        str[i] = c;
    }
    str[i] = '\0';
}

// --- BenchFHBigHashGet ---
static void BenchFHBigHashGet(benchmark::State& state)
{
    int num_strings = 1000000;
    std::vector<char *> keys(num_strings);
    char key[65];
    int err = 0;

    fh_t *fh = fh_create(num_strings, FH_DATALEN_STRING, NULL);

    for (int i = 0; i < num_strings; ++i) {
        generate_random_str(i, key, 32, 64);
        keys[i] = strdup(key);
        fh_insert(fh, keys[i], (void *)"payload");
    }

    int j = 0;
    for (auto _ : state) {
        if (j == num_strings)
        {
            j = 0;
        }
        fh_get(fh, keys[j++], &err);
    }

    fh_destroy(fh);
    for (auto ptr : keys) free(ptr);
}
BENCHMARK(BenchFHBigHashGet)->Iterations(1000)->Iterations(10000)->Iterations(100000);

// --- BenchFHBigHashGetMurmur ---
static void BenchFHBigHashGetMurmur(benchmark::State& state)
{
    int num_strings = 1000000;
    std::vector<char *> keys(num_strings);
    char key[65];
    int err = 0;

    fh_t *fh = fh_create(num_strings, FH_DATALEN_STRING, murmur64a_hash);

    for (int i = 0; i < num_strings; ++i) {
        generate_random_str(i, key, 32, 64);
        keys[i] = strdup(key);
        fh_insert(fh, keys[i], (void *)"payload");
    }

    int j = 0;
    for (auto _ : state) {
        if (j == num_strings)
        {
            j = 0;
        }
        fh_get(fh, keys[j++], &err);
    }

    fh_destroy(fh);
    for (auto ptr : keys) free(ptr);
}
BENCHMARK(BenchFHBigHashGetMurmur)->Iterations(1000)->Iterations(10000)->Iterations(100000);

BENCHMARK_MAIN();