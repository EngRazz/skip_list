#include "skip_list_rebuild.hpp"
#include <stdlib.h>
#include <limits.h>
#include <ctime>
#include <vector>

using namespace skip_list_rebuild_kernels;

/*---- Utility functions ----*/
std::vector<node_t*> search_predecessors(skip_list*s, std::int64_t k) {
    node_t *prev, *curr;
    std::vector<node_t*> predecessors;

    prev = s->get_root();
    curr = s->get_root()->next_r;
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

void insert_with_promotion(skip_list* s,std::int64_t k, std::int64_t promotion) {
    node_t *n, *new_node, *new_node_u;
    std::vector<node_t*> predecessors;

    // k must not be -inf
    if(k<=s->get_root()->k)
        return;

    predecessors = search_predecessors(s, k);
    if(predecessors.back()->k == k)
        // node alredy in the list
        return;

    // the node is not present, so it has to be inserted

    new_node=nullptr;
    new_node_u=nullptr;
    for(promotion++; promotion>0; promotion--) {
        if(!predecessors.empty()) {
            n = predecessors.back(); // retrive
            predecessors.pop_back(); // remove
        } else {
            // add new level
            n = allocate_node(s->get_root()->k);
            n->next_d = s->get_root();
            s->set_root(n); // update root
        }

        // insert node
        new_node_u = allocate_node(k);
        new_node_u->next_r = n->next_r;
        n->next_r = new_node_u;
        new_node_u->next_d = new_node;

        new_node = new_node_u;
    }

    return;
}

/*---- Skip list methods ----*/
namespace skip_list_rebuild_kernels {

    node_t* allocate_node(std::int64_t k) {
        node_t *n = new node_t{k, nullptr, nullptr};
        return n;
    }

    void free_node(node_t* n) {
        delete n;
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
        node_t* root_lv, *prev, *curr;

        // delete level by level
        root_lv = this->root;
        while(root_lv!=nullptr) {
            // delete nodes in this level
            curr = root_lv->next_r;
            while(curr!=nullptr) {
                prev = curr;
                curr = curr->next_r;
                free_node(prev);
            }
            prev = root_lv;
            root_lv = root_lv->next_d;
            free_node(prev);
        }
    }

    node_t* skip_list::get_root() {
        return this->root;
    }

    void skip_list::set_root(node_t* n) {
        this->root = n;
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

    bool skip_list::insert(std::int64_t k) {
        node_t *n, *new_node, *new_node_u;
        uint64_t promotion;
        std::vector<node_t*> predecessors;

        // k must not be -inf
        if(k<=this->root->k)
            return false;

        predecessors = search_predecessors(this, k);
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

    std::tuple<skip_list*, skip_list*> skip_list::split(std::int64_t x) {
        skip_list *l, *r;
        node_t *curr_l, *curr_r, *curr, *n;
        node_t * curr_l_u, *curr_r_u;
        node_t *root_lv_l, *root_lv_r, *root_lv;
        node_t *root_lv_l_u, *root_lv_r_u;

        l = new skip_list(this->seed);
        r = new skip_list(this->seed);

        root_lv = this->root;
        root_lv_l = l->root;
        root_lv_r = r->root;
        root_lv_l_u = nullptr;
        root_lv_r_u = nullptr;
        while (root_lv!=nullptr) {
            curr = root_lv->next_r;
            curr_l = root_lv_l;
            curr_r = root_lv_r;
            curr_l_u = root_lv_l_u ? root_lv_l_u->next_r : nullptr;
            curr_r_u = root_lv_r_u ? root_lv_r_u->next_r : nullptr;
            while(curr!=nullptr) {
                n = allocate_node(curr->k);
                if(curr->k < x) {
                    curr_l->next_r = n;
                    curr_l = n;
                    if(curr_l_u!=nullptr && curr_l_u->k == curr->k) {
                        curr_l_u->next_d = n;
                        curr_l_u = curr_l_u->next_r;
                    }

                } else {
                    curr_r->next_r = n;
                    curr_r = n;
                    if(curr_r_u!=nullptr && curr_r_u->k == curr->k) {
                        curr_r_u->next_d = n;
                        curr_r_u = curr_r_u->next_r;
                    }
                }
                curr = curr->next_r;
            }

            root_lv = root_lv->next_d;
            if(root_lv!=nullptr) {
                root_lv_l_u = root_lv_l;
                root_lv_r_u = root_lv_r;
                root_lv_l = allocate_node(this->root->k);
                root_lv_r = allocate_node(this->root->k);
                root_lv_l_u->next_d = root_lv_l;
                root_lv_r_u->next_d = root_lv_r;
            }
        }

        return {l, r};
    }

    node_t* search(skip_list* s, std::int64_t k) {
        return s->search(k);
    }

    bool insert(skip_list* s, std::int64_t k) {
        return s->insert(k);
    }

    skip_list* ordered_union(skip_list* a, skip_list* b) {
        skip_list* s;
        std::uint64_t promotion;
        node_t *root_lv, *curr, *curr_d;

        s = new skip_list(a->get_seed());

        root_lv = a->get_root();
        while(root_lv!=nullptr) {
            curr = root_lv->next_r;
            while(curr!=nullptr) {
                if(curr->k!=s->get_root()->k) { // detect if a node has already been visited
                    // compte promotion
                    promotion = 0;
                    curr_d = curr->next_d;
                    while(curr_d!=nullptr) {
                        promotion++;
                        curr_d->k = s->get_root()->k; // so it will detect already visited nodes
                        curr_d = curr_d->next_d;
                    }

                    insert_with_promotion(s, curr->k, promotion);
                }
                curr = curr->next_r;
            }
            root_lv = root_lv->next_d;
        }

        root_lv = b->get_root();
        while(root_lv!=nullptr) {
            curr = root_lv->next_r;
            while(curr!=nullptr) {
                // compte promotion
                promotion = 0;
                curr_d = curr->next_d;
                while(curr_d!=nullptr) {
                    promotion++;
                    curr_d = curr_d->next_d;
                }

                insert_with_promotion(s, curr->k, promotion);
                curr = curr->next_r;
            }
            root_lv = root_lv->next_d;
        }

        delete a;
        delete b;
        return s;
    }

    std::tuple<skip_list*, skip_list*> split(skip_list* s, std::int64_t x) {
        std::tuple<skip_list*, skip_list*> result;

        result = s->split(x);
        delete s;
        return result;
    }

}
