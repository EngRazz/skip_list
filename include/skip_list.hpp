#ifndef SKIP_LIST_H
#define SKIP_LIST_H

#include <cstdint>
#include <tuple>
#include <random>

namespace skip_list_kernels {

    struct random_handler {
        std::uint32_t seed;
        std::mt19937 generator;
        std::bernoulli_distribution promote{0.5};
    };

    struct node_t {
        std::int64_t k;

        node_t* next_r = nullptr; // next node right, on the list
        node_t* next_d = nullptr; // next node down, in the skip list
    };

    /*
     * Allocate a new node.
     */
    node_t* allocate_node(std::int64_t k);

    /*
     * Free the node n.
     */
    void free_node(node_t* n);

    class skip_list {
        private:
            node_t* root; // first node, with key = -infinity (min int64)
            random_handler* rh;

        public:
            skip_list(random_handler* rh);
            ~skip_list();
            skip_list(const skip_list&) = delete;
            skip_list& operator=(const skip_list&) = delete;
            node_t* get_root();
            /*
             * It does not free the old node.
             */
            void set_root(node_t* n);
            random_handler* get_rh();
            node_t* search(std::int64_t k);
            bool insert(std::int64_t k);
            std::tuple<skip_list*, skip_list*> split(std::int64_t x);
            std::tuple<skip_list*, skip_list*> split_rebuild(std::int64_t x);
    };

    /*
     * Return wheter a node with key equal to k
     * is present in the list. The list is left
     * unchanged.
     */
    node_t* search(skip_list* s, std::int64_t k);

    /*
     * Insert a node with key equal to k, only
     * if absent. A duplicate doee not change
     * the height and the set.
     */
    bool insert(skip_list* s, std::int64_t k);

    /*
     * Return all keys from a and b. Inputs must
     * be distinct; when both are nonempty,
     * every key in a must be strictly smaller
     * than every key in b. Empty inputs are valid.
     * Consume a and b: so they will be empty skip list.
     */
    skip_list* ordered_union(skip_list* a, skip_list* b);

    /*
     * Return L = {k in S : k ≤ x} and R = {k in S : k > x}.
     * The pivot need not be present. Consume S: so it will
     * br and empty skip list.
     */
    std::tuple<skip_list*, skip_list*> split(skip_list* s, std::int64_t x);

    /*
     *
     */
    std::tuple<skip_list*, skip_list*> split_rebuild(skip_list* s, std::int64_t x);
}

#endif // SKIP_LIST_H

