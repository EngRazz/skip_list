#ifndef BENCHMARK_SUPPORT_H
#define BENCHMARK_SUPPORT_H

#include "skip_list.hpp"
#include <cstdint>

namespace skip_list_benchmarks {

    inline constexpr std::int64_t sizes[] = {
        1024, 4096, 16384, 65536, 262144, 1048576
    };
    inline constexpr std::uint32_t height_seeds[] = {42, 900, 5783};
    inline constexpr int repetitions = 3;
    inline constexpr int search_batch_size = 256;
    inline constexpr int insertion_batch_size = 32;
    inline constexpr int mutation_iterations = 10;
    inline constexpr std::uint32_t workload_seed = 17;

    void register_search_benchmarks();
    void register_insertion_benchmarks();
    void register_union_benchmarks();
    void register_split_benchmarks();

    /*---- Untimed input construction and checks ----*/

    inline void fill_even_keys(skip_list_kernels::skip_list* s,
                               std::int64_t first, std::int64_t count) {
        for(std::int64_t i=0; i<count; i++)
            s->insert(2*(first+i));
    }

    inline bool has_even_keys(skip_list_kernels::skip_list* s,
                              std::int64_t first, std::int64_t count) {
        if(s==nullptr || s->get_root()==nullptr)
            return false;

        skip_list_kernels::node_t* curr = s->get_root();
        while(curr->next_d!=nullptr)
            curr = curr->next_d;
        curr = curr->next_r;

        for(std::int64_t i=0; i<count; i++) {
            if(curr==nullptr || curr->k!=2*(first+i))
                return false;
            curr = curr->next_r;
        }
        return curr==nullptr;
    }

}

#endif // BENCHMARK_SUPPORT_H
