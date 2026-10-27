#pragma once

#include "implementation.hpp"
#include "runner_options.hpp"

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace tests {

using Key = std::int64_t;
using Seed = std::uint32_t;
using Keys = std::vector<Key>;
using KeySet = std::set<Key>;
using OwnedList = std::unique_ptr<List>;
constexpr Key minimum = std::numeric_limits<Key>::min();
constexpr Key maximum = std::numeric_limits<Key>::max();

struct Case {
    std::string name;
    std::function<void(Seed)> check;
};

struct Snapshot {
    Node* root;
    Keys keys;
    std::map<Key, std::vector<Node*>> towers;
    std::unordered_map<Node*, std::tuple<Key, Node*, Node*>> links;
};

struct Partition {
    OwnedList left;
    OwnedList right;
};

void require(bool condition, const std::string& message);
Snapshot inspect(List& list);
bool same_structure(const Snapshot& before, const Snapshot& after);
void expect_contents(List& list, const KeySet& expected);
void expect_separate(List& a, List& b);
void expect_preserved(const Snapshot& before, const Snapshot& after);
void expect_reusable(List& input, std::initializer_list<List*> outputs);
void fill(List& list, const Keys& keys);
OwnedList unite(List& a, List& b);
Partition partition(List& input, Key pivot, bool rebuild = false);
void expect_next_random_insert(List& list, Random& random, Key key);
bool run_case(const Case& test, Seed seed);

std::vector<Case> insertion_cases();
std::vector<Case> search_cases();
std::vector<Case> union_cases();
std::vector<Case> split_cases();
std::vector<Case> split_rebuild_cases();
std::vector<Case> mixed_cases();
std::vector<Case> mixed_rebuild_cases();

} // namespace tests
