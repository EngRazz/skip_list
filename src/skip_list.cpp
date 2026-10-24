#include "skip_list.hpp"
#include <stdlib.h>
#include <limits.h>
#include <time.h>

namespace skip_list_kernels {

    node_t* allocate_node(std::int64_t k) {
        node_t *n = (node_t*)std::malloc(sizeof(node_t));
        if(n==nullptr)
            exit(1);
        n->ref_count = 1;
        n->k = k;
        n->next_r = nullptr;
        n->next_d = nullptr;
        return n;
    }

    void retain_node(node_t* n) {
        if(n==nullptr || n->ref_count<1)
            exit(2);
    (n->ref_count)++;
    }

    void release_node(node_t* n) {
        if(n==nullptr)
            return;
        (n->ref_count)++;
        if(n->ref_count < 1)
            free_node(n);
    }

    void free_node(node_t* n) {
        if(n!=nullptr)
            std::free(n);
    }

    skip_list::skip_list() {
        time_t t;
        std::int64_t neg_inf = std::numeric_limits<std::int64_t>::min();
        this->root = allocate_node(neg_inf);
        this->seed = std::time(&t);
        this->generator.seed(this->seed);
    }

    skip_list::skip_list(std::uint32_t seed) {
        std::int64_t neg_inf = std::numeric_limits<std::int64_t>::min();
        this->root = allocate_node(neg_inf);
        this->seed = seed;
        this->generator.seed(seed);
    }

    skip_list::~skip_list() {
        // TODO: implement
    }

    std::uint32_t skip_list::get_seed() {
        return this->seed;
    }

    void skip_list::set_seed(std::uint32_t seed) {
        this->seed = seed;
        this->generator.seed(seed);
    }

    node_t* skip_list::search(std::int64_t k) {
        node_t *prev, *curr;

        prev = this->root;
        curr = prev->next_r;
        while(1) {
            if(curr==nullptr) {
                // move below
                if(prev->next_d==nullptr) // prev is never null since it starts as root
                    // no level below, so element is not present
                    return nullptr;

                prev = prev->next_d;
                curr = prev->next_r;

            } else {

                if(k == curr->k)
                    // element found
                    return curr;

                if(k < curr->k) {
                    // move below
                    if(prev->next_d==nullptr)  // prev is never null since it starts as root

                        // no level below, so element is not present
                        return nullptr;

                    prev = prev->next_d;
                    curr = prev->next_r;
                } else {
                    prev = curr;
                    curr = curr->next_r;
                }
            }
        }
    }

    std::vector<node_t*> skip_list::search_predecessors(std::int64_t k) {
        node_t *prev, *curr;
        std::vector<node_t*> predecessors;

        prev = this->root;
        curr = prev->next_r;
        while(1) {
            if(curr==nullptr) {
                // move below
                if(prev->next_d==nullptr) { // prev is never null since it starts as root
                    // no level below, so element is not present
                    predecessors.push_back(prev);
                    return predecessors;
                }

                predecessors.push_back(prev);
                prev = prev->next_d;
                curr = prev->next_r;

            } else {

                if(k == curr->k) {
                    // element found
                    predecessors.push_back(curr);
                    return predecessors;
                }

                if(k < curr->k) {
                    // move below
                    if(prev->next_d==nullptr) { // prev is never null since it starts as root
                        // no level below, so element is not present
                        predecessors.push_back(prev);
                        return predecessors;
                    }

                    predecessors.push_back(prev);
                    prev = prev->next_d;
                    curr = prev->next_r;
                } else {
                    prev = curr;
                    curr = curr->next_r;
                }
            }
        }
    }

    bool skip_list::insert(std::int64_t k) {
        node_t *n, *new_node, *new_node_u;
        uint64_t promotion;
        std::vector<node_t*> predecessors;

        // k must not be -inf
        if(k<=this->root->k)
            return false;

        predecessors = search_predecessors(k);
        if(predecessors.back()->k == k)
            // node alredy in the list
            return false;

        // the node is not present, so it has to be inserted

        // random promotion
        promotion = 0;
        while(promote(generator))
            promotion++;

        new_node=nullptr;
        new_node_u=nullptr;
        for(promotion++; promotion>0; promotion--) {
            if(!predecessors.empty()) {
                n = predecessors.back(); // retrive
                predecessors.pop_back(); // remove
            } else {
                // add new level
                n = allocate_node(this->root->k);
                n->next_d = this->root;
                this->root = n; // update root
            }

            // insert node
            new_node_u = allocate_node(k);
            new_node_u->next_r = n->next_r;
            n->next_r = new_node_u;
            new_node_u->next_d = new_node;

            new_node = new_node_u;
        }

        return  true;
    }

    std::tuple<skip_list*, skip_list*> split(std::int64_t x) {
        // TODO: implement
    }

    node_t* search(skip_list* s, std::int64_t k) {
        return s->search(k);
    }

    bool insert(skip_list* s, std::int64_t k) {
        return s->insert(k);
    }

    skip_list* ordered_union(skip_list* a, skip_list* b) {
        // TODO: Implement
        return nullptr;
    }

    std::tuple<skip_list*, skip_list*> split(skip_list* s, std::int64_t x) {
        return s->split(x);
    }

}
