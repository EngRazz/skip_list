#include "benchmark_support.hpp"
#include <benchmark/benchmark.h>
#include <random>
#include <string>

using namespace skip_list_kernels;
using namespace skip_list_benchmarks;

/*---- Ordered union benchmark ----*/

static void BM_OrderedUnion(benchmark::State& state, std::uint32_t seed) {
    std::int64_t n = state.range(0);

    for(auto _ : state) {
        state.PauseTiming();
        bool valid;
        {
            random_handler rh{seed, std::mt19937(seed)};
            skip_list a(&rh), b(&rh);
            skip_list* result;

            // A and B use successive draws from the same handler.
            fill_even_keys(&a, 0, n/2);
            fill_even_keys(&b, n/2, n/2);

            state.ResumeTiming();
            result = ordered_union(&a, &b);
            benchmark::DoNotOptimize(result);
            benchmark::ClobberMemory();
            state.PauseTiming();

            valid = has_even_keys(&a, 0, 0) && has_even_keys(&b, 0, 0)
                && has_even_keys(result, 0, n);
            delete result;
        } // The consumed input objects are destroyed while timing is paused.
        if(!valid) {
            state.SkipWithError("incorrect union output or nonempty consumed inputs");
            return;
        }
        state.ResumeTiming();
    }

    state.SetComplexityN(n);
    state.SetItemsProcessed(state.iterations());
    state.counters["N"] = n;
    state.counters["height_seed"] = seed;
    state.counters["operations_per_iteration"] = 1;
}

namespace skip_list_benchmarks {

    void register_union_benchmarks() {
        for(std::uint32_t seed : height_seeds) {
            std::string name = "ordered_union/seed_" + std::to_string(seed);
            auto* b = benchmark::RegisterBenchmark(name.c_str(), BM_OrderedUnion, seed);

            for(std::int64_t n : sizes)
                b->Arg(n);
            b->ArgName("N")->Unit(benchmark::kNanosecond)
                ->Iterations(mutation_iterations)->Repetitions(repetitions)->Complexity(benchmark::oLogN);
        }
    }

}
