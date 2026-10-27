#include "benchmark_support.hpp"
#include <benchmark/benchmark.h>
#include <algorithm>
#include <array>
#include <random>
#include <string>
#include <vector>

using namespace skip_list_kernels;
using namespace skip_list_benchmarks;

/*---- Workload generation ----*/

static std::vector<std::int64_t> make_insertion_keys(std::int64_t n) {
    std::mt19937 generator(workload_seed);
    std::vector<std::int64_t> keys;
    std::int64_t first, last;

    // One absent odd key from each of 32 disjoint parts of the range.
    for(int i=0; i<insertion_batch_size; i++) {
        first = i*n/insertion_batch_size;
        last = (i+1)*n/insertion_batch_size - 1;
        std::uniform_int_distribution<std::int64_t> position(first, last);
        keys.push_back(2*position(generator) + 1);
    }
    std::shuffle(keys.begin(), keys.end(), generator);
    return keys;
}

/*---- Insertion benchmark ----*/

static void BM_Insertion(benchmark::State& state, std::uint32_t seed) {
    std::int64_t n = state.range(0);
    std::vector<std::int64_t> keys = make_insertion_keys(n);

    for(auto _ : state) {
        state.PauseTiming();
        bool valid = true;
        {
            random_handler rh{seed, std::mt19937(seed)};
            skip_list s(&rh);
            std::array<bool, insertion_batch_size> inserted;
            fill_even_keys(&s, 0, n);

            state.ResumeTiming();
            for(int i=0; i<insertion_batch_size; i++) {
                inserted[i] = insert(&s, keys[i]);
                benchmark::DoNotOptimize(inserted[i]);
            }
            benchmark::ClobberMemory();
            state.PauseTiming();

            for(int i=0; i<insertion_batch_size; i++) {
                node_t* result = search(&s, keys[i]);
                if(!inserted[i] || result==nullptr || result->k!=keys[i])
                    valid = false;
            }
        } // Destruction is untimed; the next iteration starts with N keys again.
        if(!valid) {
            state.SkipWithError("a timed insertion failed");
            return;
        }
        state.ResumeTiming();
    }

    state.SetComplexityN(n);
    state.SetItemsProcessed(state.iterations()*insertion_batch_size);
    state.counters["N"] = n;
    state.counters["height_seed"] = seed;
    state.counters["workload_seed"] = workload_seed;
    state.counters["operations_per_iteration"] = insertion_batch_size;
}

namespace skip_list_benchmarks {

    void register_insertion_benchmarks() {
        for(std::uint32_t seed : height_seeds) {
            std::string name = "insertion/seed_" + std::to_string(seed);
            auto* b = benchmark::RegisterBenchmark(name.c_str(), BM_Insertion, seed);

            for(std::int64_t n : sizes)
                b->Arg(n);
            b->ArgName("N")->Unit(benchmark::kNanosecond)
                ->Iterations(mutation_iterations)->Repetitions(repetitions)->Complexity(benchmark::oLogN);
        }
    }

}
