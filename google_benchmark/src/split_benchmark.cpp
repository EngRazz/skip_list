#include "benchmark_support.hpp"
#include <benchmark/benchmark.h>
#include <random>
#include <string>
#include <tuple>

using namespace skip_list_kernels;
using namespace skip_list_benchmarks;

/*---- Split benchmark ----*/

static void BM_Split(benchmark::State& state, std::uint32_t seed, bool rebuild) {
    std::int64_t n = state.range(0);
    std::int64_t pivot = n-1; // Absent odd key; each output gets N/2 even keys.

    for(auto _ : state) {
        state.PauseTiming();
        bool valid, correct_randomness;
        {
            random_handler rh{seed, std::mt19937(seed)};
            skip_list s(&rh);
            std::tuple<skip_list*, skip_list*> result;
            fill_even_keys(&s, 0, n);
            auto generator = rh.generator;

            state.ResumeTiming();
            result = rebuild ? split_rebuild(&s, pivot) : split(&s, pivot);
            benchmark::DoNotOptimize(result);
            benchmark::ClobberMemory();
            state.PauseTiming();

            skip_list* l = std::get<0>(result);
            skip_list* r = std::get<1>(result);
            valid = has_even_keys(&s, 0, 0)
                && has_even_keys(l, 0, n/2) && has_even_keys(r, n/2, n/2);
            correct_randomness = rebuild ? rh.generator != generator : rh.generator == generator;
            delete l;
            delete r;
        }
        if(!valid) {
            state.SkipWithError("incorrect split outputs or nonempty consumed input");
            return;
        }
        if(!correct_randomness) {
            state.SkipWithError(rebuild ? "split_rebuild did not draw fresh heights; check its wrapper"
                                        : "structural split changed the height generator");
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

    void register_split_benchmarks() {
        for(bool rebuild : {false, true}) {
            for(std::uint32_t seed : height_seeds) {
                std::string name = rebuild ? "split_rebuild/seed_" : "split/seed_";
                name += std::to_string(seed);
                auto* b = benchmark::RegisterBenchmark(name.c_str(), BM_Split, seed, rebuild);

                for(std::int64_t n : sizes)
                    b->Arg(n);
                b->ArgName("N")->Unit(benchmark::kNanosecond)
                    ->Iterations(mutation_iterations)->Repetitions(repetitions);
                if(rebuild)
                    b->Complexity(benchmark::oNLogN);
                else
                    b->Complexity(benchmark::oLogN);
            }
        }
    }

}
