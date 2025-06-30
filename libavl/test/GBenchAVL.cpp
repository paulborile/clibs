#include <benchmark/benchmark.h>
#include <vector>
#include <string>
#include <random>

// C headers
#include <cstring>

// From libavl
#include "avl.h"

// =============================================================================
// Helper Functions
// =============================================================================

// A more robust random string generator using C++11 <random>
void generate_random_str_cpp(std::mt19937& gen, char *str, int len)
{
    static const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    // The -2 is because sizeof includes the null terminator
    std::uniform_int_distribution<int> dist(0, sizeof(charset) - 2);
    for (int i = 0; i < len; ++i) {
        str[i] = charset[dist(gen)];
    }
    str[len] = '\0';
}

// =============================================================================
// Google Benchmark Fixture
// =============================================================================

// Use a standard fixture. Parameters will be passed via registration macros.
class AVLBenchmark : public benchmark::Fixture {
public:
    static constexpr int string_len = 32;

    avl_h *avl = nullptr;
    std::vector<std::string> keys_storage;
    std::vector<char *> keys_view;
    // This will be set from state.range(0) in SetUp
    long long num_items = 0;

    void SetUp(const ::benchmark::State& state) override
    {
        num_items = state.range(0);
        // If num_items is invalid, we just return. The benchmark functions
        // will check this and skip themselves.
        if (num_items <= 0)
        {
            return;
        }

        // Use std::vector<std::string> for safe, automatic memory management.
        // This avoids the stack overflow from the original `char* keys[1000000]`.
        keys_storage.resize(num_items);
        keys_view.resize(num_items);

        std::mt19937 gen(12345); // Fixed seed for reproducibility

        for (int i = 0; i < num_items; ++i) {
            keys_storage[i].resize(string_len + 1);
            generate_random_str_cpp(gen, &keys_storage[i][0], string_len);
            keys_view[i] = &keys_storage[i][0];
        }

        avl = avl_create(nullptr);
        // If creation fails, avl will be nullptr. The benchmark functions
        // must handle this.
        if (!avl)
        {
            return;
        }

        // Pre-fill the tree for search benchmarks
        for (int i = 0; i < num_items; ++i) {
            // Using the key itself as the "data" pointer for simplicity
            avl_insert(avl, keys_view[i], keys_view[i]);
        }
    }

    void TearDown(const ::benchmark::State& /*state*/) override
    {
        if (avl)
        {
            avl_destroy(avl);
            avl = nullptr;
        }
        // Vector destructors will handle all key memory automatically.
    }
};

// Define the benchmark logic using the fixture.
BENCHMARK_DEFINE_F(AVLBenchmark, SearchHit)(benchmark::State& state)
{
    // Per Google Benchmark rules, setup errors must be checked inside the
    // benchmark function itself, where `state` is not const.
    if (num_items <= 0)
    {
        state.SkipWithError("Number of items must be positive.");
        return;
    }
    if (!avl)
    {
        state.SkipWithError("Failed to create avl_h during setup.");
        return;
    }
    long long i = 0;
    for (auto _ : state) {
        void *result = avl_search(avl, keys_view[i]);
        benchmark::DoNotOptimize(result);
        i = (i + 1) % num_items; // Correctly cycle through keys
    }
    state.SetItemsProcessed(state.iterations());
}

// Define the benchmark logic for a cache miss.
BENCHMARK_DEFINE_F(AVLBenchmark, SearchMiss)(benchmark::State& state)
{
    if (num_items <= 0)
    {
        state.SkipWithError("Number of items must be positive.");
        return;
    }
    if (!avl)
    {
        state.SkipWithError("Failed to create avl_h during setup.");
        return;
    }

    char non_existent_key[] = "this_key_will_not_be_found_in_the_tree";
    for (auto _ : state) {
        void *result = avl_search(avl, non_existent_key);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations());
}

// Register the fixture-based benchmarks to run with different tree sizes.
BENCHMARK_REGISTER_F(AVLBenchmark, SearchHit)->Arg(200)->Arg(100000)->Arg(1000000);
BENCHMARK_REGISTER_F(AVLBenchmark, SearchMiss)->Arg(200)->Arg(100000)->Arg(1000000);

BENCHMARK_MAIN();