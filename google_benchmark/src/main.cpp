#include "benchmark_support.hpp"
#include <benchmark/benchmark.h>

using namespace skip_list_benchmarks;

int main(int argc, char** argv) {
    register_search_benchmarks();
    register_insertion_benchmarks();
    register_union_benchmarks();
    register_split_benchmarks();

    benchmark::Initialize(&argc, argv);
    if(benchmark::ReportUnrecognizedArguments(argc, argv))
        return 1;

    benchmark::AddCustomContext("implementation", "skip_list.cpp");
    benchmark::AddCustomContext("complexity_model", "supplied log2(N), no intercept");
    benchmark::AddCustomContext("split_rebuild_complexity_model", "supplied N log2(N), no intercept");
    benchmark::AddCustomContext("search_workload", "128 hits and 128 misses per iteration");
    benchmark::AddCustomContext("insertion_workload", "32 absent keys; starting size N each iteration");
    benchmark::AddCustomContext("union_workload", "ordered disjoint inputs of N/2 keys each");
    benchmark::AddCustomContext("split_workload", "N even keys; absent pivot N-1; equal outputs");
    benchmark::AddCustomContext("split_rebuild_workload", "same split inputs; ordinary insertions and consumed-node disposal timed");
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
