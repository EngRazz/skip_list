#include "benchmark_support.hpp"
#include "skip_list.hpp"
#include <benchmark/benchmark.h>
#include <algorithm>
#include <random>
#include <string>
#include <vector>

using namespace skip_list_kernels;
using namespace skip_list_benchmarks;

/*---- Workload generation ----*/

/*
 * Stored keys are even. Each part of the key range contributes
 * one present even key and one absent odd key.
 */
static std::vector<std::int64_t> make_search_queries(std::int64_t n) {
    std::mt19937 generator(workload_seed);
    std::vector<std::int64_t> queries;
    std::int64_t first, last, index;
    int pairs = search_batch_size/2;

    queries.reserve(search_batch_size);
    for(int i=0; i<pairs; i++) {
        first = i*n/pairs;
        last = (i+1)*n/pairs - 1;
        std::uniform_int_distribution<std::int64_t> position(first, last);
        index = position(generator);
        queries.push_back(2*index);
        queries.push_back(2*index + 1);
    }
    std::shuffle(queries.begin(), queries.end(), generator);
    return queries;
}

/*---- Search benchmark ----*/

static void BM_Search(benchmark::State& state, std::uint32_t seed) {
    std::int64_t n = state.range(0);
    random_handler rh{seed, std::mt19937(seed)};
    skip_list s(&rh);
    node_t* result;

    // Google Benchmark starts timing when the state loop begins.
    for(std::int64_t i=0; i<n; i++)
        insert(&s, 2*i);
    std::vector<std::int64_t> queries = make_search_queries(n);

    // A small untimed check also warms the query paths.
    for(std::int64_t k : queries) {
        result = search(&s, k);
        if((result!=nullptr) != (k%2==0) || (result!=nullptr && result->k!=k)) {
            state.SkipWithError("incorrect search result before timing");
            return;
        }
    }

    for(auto _ : state) {
        for(std::int64_t k : queries) {
            result = search(&s, k);
            benchmark::DoNotOptimize(result);
        }
    }

    state.SetComplexityN(n);
    state.SetItemsProcessed(state.iterations()*search_batch_size);
    state.counters["N"] = n;
    state.counters["height_seed"] = seed;
    state.counters["workload_seed"] = workload_seed;
    state.counters["searches_per_iteration"] = search_batch_size;
    state.counters["operations_per_iteration"] = search_batch_size;
}

namespace skip_list_benchmarks {

    void register_search_benchmarks() {
        // Each seed gets its own fit across the six sizes.
        for(std::uint32_t seed : height_seeds) {
            std::string name = "search/seed_" + std::to_string(seed);
            auto* b = benchmark::RegisterBenchmark(name.c_str(), BM_Search, seed);

            for(std::int64_t n : sizes)
                b->Arg(n);
            b->ArgName("N")->Unit(benchmark::kNanosecond)
                ->Repetitions(repetitions)->Complexity(benchmark::oLogN);
        }
    }

}
