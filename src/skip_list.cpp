#include "skip_list.hpp"
#include <stdlib.h>
#include <limits.h>
#include <ctime>
#include <vector>

using namespace skip_list_kernels;

/*---- Utility functions ----*/

/*
 * Returns a list of all the nodes that
 * lead from the root to the insertion position.
 * Those are required to perform promotions or splitting.
 */
std::vector<node_t*> search_predecessors(skip_list* s, std::int64_t k) {
    node_t *prev, *curr;
    std::vector<node_t*> predecessors;

    prev = s->get_root();
    curr = s->get_root()->next_r;
    while(1) {
        if(curr==nullptr) {
            // move below
            predecessors.push_back(prev);
            // prev is never null since it starts as root
            if(prev->next_d==nullptr)
                // no level below, so element is not present
                return predecessors;

            // move right on the next level
            prev = prev->next_d;
            curr = prev->next_r;

        } else {

            // this check is required since the methos is also used in
            // the split, where x goes to the left list
            if(k == curr->k) {
                // element found
                predecessors.push_back(curr);
                return predecessors;
            }

            if(k < curr->k) {
                // move below
                predecessors.push_back(prev);
                // prev is never null since it starts as root
                if(prev->next_d==nullptr)
                    // no level below, so element is not present
                    return predecessors;

                // move right in the next level
                prev = prev->next_d;
                curr = prev->next_r;
            } else {
                prev = curr;
                curr = curr->next_r;
            }
        }
    }
}

void consume_skip_list(skip_list* s) {
    node_t* root_lv, *prev, *curr;

    root_lv = s->get_root();
    curr = root_lv->next_r;
    while(curr!=nullptr) {
        prev = curr;
        curr = curr->next_r;
        free_node(prev);
    }
    root_lv = root_lv->next_d;
    s->get_root()->next_r = nullptr;
    s->get_root()->next_d = nullptr;
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

/*---- Skip list methods ----*/
namespace skip_list_kernels {

    node_t* allocate_node(std::int64_t k) {
        node_t *n = new node_t{k, nullptr, nullptr};
        return n;
    }

    void free_node(node_t* n) {
        delete n;
    }

    skip_list::skip_list(random_handler* rh) {
        time_t t;
        std::int64_t neg_inf = std::numeric_limits<std::int64_t>::min();
        this->root = allocate_node(neg_inf);
        this->rh = rh;
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

    random_handler* skip_list::get_rh() {
        return this->rh;
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
        node_t *n, *new_node, *new_node_u; uint64_t promotion;
        std::vector<node_t*> predecessors;

        // k must not be present (-inf does not count)
        if(this->search(k)!=nullptr)
            return false;

        predecessors = search_predecessors(this, k);

        // random promotion
        promotion = 0;
        while(rh->promote(rh->generator))
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
        std::vector<node_t*> predecessors;
        node_t *root_lv, *root_lv_u, *n;

        l = new skip_list(this->rh);
        r = new skip_list(this->rh);

        predecessors = search_predecessors(this, x); // this vector is never empty

        // if x in the list, predecessors will contain
        // only its upper node; to split I need to go
        // down to the bottom, so the height is preserved
        n = predecessors.back();
        if(n!=nullptr && n->k == x) {
            n = n->next_d;
            while(n!=nullptr) {
                predecessors.push_back(n);
                n = n->next_d;
            }
        }

        root_lv = nullptr;
        root_lv_u = nullptr;
        while(!predecessors.empty()) {
            n = predecessors.back();
            predecessors.pop_back();
            if(n->next_r!=nullptr) {
                root_lv_u = allocate_node(this->root->k);
                root_lv_u->next_r = n->next_r;
                root_lv_u->next_d = root_lv;
                root_lv = root_lv_u;
                n->next_r = nullptr;

            } else
                // all the other nodes in predecessors
                // have k <= n->k; if at this level
                // there is nothing on the right, then
                // there will be nothing on the right
                // even in all the upper levels
                break;
        }

        l->root->next_r = this->root->next_r;
        l->root->next_d = this->root->next_d;
        this->root->next_r = nullptr;
        this->root->next_d = nullptr;
        // remove empty upper levels from l
        while(l->root->next_r == nullptr && l->root->next_d != nullptr) {
            root_lv = l->root;
            l->root = l->root->next_d;
            free_node(root_lv);
        }

        if(root_lv_u!=nullptr) {
            // there is at least a node on the
            // right of the "border" defined by
            // nodes inside predecessors
            r->root->next_r = root_lv_u->next_r;
            r->root->next_d = root_lv_u->next_d;
            free_node(root_lv_u);
        }

        return {l, r};
    }

    std::tuple<skip_list*, skip_list*> skip_list::split_rebuild(std::int64_t x) {
        node_t *curr;
        skip_list *l, *r;

        l = new skip_list(this->get_rh());
        r = new skip_list(this->get_rh());

        // go to the lowest level
        curr = this->root;
        while(curr->next_d!=nullptr)
            curr = curr->next_d;

        // insert nodes
        curr = curr->next_r;
        while(curr!=nullptr) {
            if(curr->k <= x)
                l->insert(curr->k);
            else
                r->insert(curr->k);
            curr = curr->next_r;
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
        std::vector<node_t*> border_a;
        std::vector<node_t*> border_b;
        node_t *curr_a, *curr_b, *tmp;

        if(a->get_rh() != b->get_rh())
            exit(1);
        s = new skip_list(a->get_rh());

        // find the "right border" of a
        curr_a = a->get_root();
        while(curr_a!=nullptr) {
            if(curr_a->next_r==nullptr) {
                border_a.push_back(curr_a);
                curr_a = curr_a->next_d;
            } else
                curr_a = curr_a->next_r;
        }

        // find the "left border" of b
        // (a list of all the level roots)
        curr_b = b->get_root();
        while(curr_b!=nullptr) {
            border_b.push_back(curr_b);
            curr_b = curr_b->next_d;
        }

        while(!border_a.empty() && !border_b.empty()) {
            curr_a = border_a.back();
            border_a.pop_back();
            curr_b = border_b.back();
            border_b.pop_back();

            curr_a->next_r = curr_b->next_r;
            curr_b->next_r = nullptr;
        }

        if(border_b.empty()) {
            // use a's root
            s->get_root()->next_r = a->get_root()->next_r;
            s->get_root()->next_d = a->get_root()->next_d;

            // delete old danglin b's level roots
            curr_b = b->get_root()->next_d;
            while(curr_b!=nullptr) {
                tmp = curr_b;
                curr_b = curr_b->next_d;
                free_node(tmp);
            }
        } else {
            // use b's root

            // connect b's root levels with a's root
            curr_a = allocate_node(a->get_root()->k);
            curr_a->next_r = a->get_root()->next_r;
            curr_a->next_d = a->get_root()->next_d;
            // delete old danglin b's level roots
            curr_b = border_b.back();
            curr_b = curr_b->next_d;
            while(curr_b!=nullptr) {
                tmp = curr_b;
                curr_b = curr_b->next_d;
                free_node(tmp);
            }
            curr_b = border_b.back();
            curr_b->next_d = curr_a;

            s->get_root()->next_r = b->get_root()->next_r;
            s->get_root()->next_d = b->get_root()->next_d;
        }
        a->get_root()->next_r = nullptr;
        a->get_root()->next_d = nullptr;
        b->get_root()->next_r = nullptr;
        b->get_root()->next_d = nullptr;

        return s;
    }

    std::tuple<skip_list*, skip_list*> split(skip_list* s, std::int64_t x) {
        std::tuple<skip_list*, skip_list*> result;

        result = s->split(x);
        return result;
    }

    std::tuple<skip_list*, skip_list*> split_rebuild(skip_list* s, std::int64_t x) {
        std::tuple<skip_list*, skip_list*> result;

        result = s->split_rebuild(x);
        consume_skip_list(s);
        return result;
    }
} // namespace skip_list_kernels
