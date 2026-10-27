#include "test_support.hpp"

namespace tests {

static void check_searches(const Keys& keys, const Keys& queries, Seed seed) {
    Random random{seed, std::mt19937(seed)};
    List list(&random);
    fill(list, keys);
    KeySet expected(keys.begin(), keys.end());
    auto before = inspect(list);
    auto generator = random.generator;
    for (Key query : queries) {
        Node* found = implementation::search(&list, query);
        require((found != nullptr) == (expected.count(query) != 0), "wrong search result");
        if (found) {
            const auto& tower = before.towers.at(query);
            require(std::find(tower.begin(), tower.end(), found) != tower.end(), "search returned a sentinel or wrong node");
        }
        require(same_structure(before, inspect(list)), "search changed the structure");
        require(random.generator == generator, "search changed the generator");
    }
}

std::vector<Case> search_cases() {
    struct Input { std::string name; Keys keys; Keys queries; };
    Keys many_keys, queries;
    for (int i = 0; i < 1000; ++i) many_keys.push_back(2 * i);
    for (int i = 0; i < 2000; ++i) queries.push_back(i);
    const std::vector<Input> inputs = {
        {"empty list, including INT64_MIN", {}, {minimum, -1, 0, 1}},
        {"singleton hits and misses", {5}, {4, 5, 6}},
        {"INT64_MIN singleton", {minimum}, {minimum, minimum + 1, maximum}},
        {"present negative, zero, and positive keys", {-10, -4, 0, 8, 20}, {-10, -4, 0, 8, 20}},
        {"absent keys before, between, and after stored keys", {-10, 0, 20}, {-11, -5, 1, 21}},
        {"both int64 boundaries", {minimum, minimum + 1, maximum}, {minimum, minimum + 1, minimum + 2, maximum - 1, maximum}},
        {"absent INT64_MIN does not match a sentinel", {minimum + 1, 0, maximum}, {minimum}},
        {"2000 queries: half hits and half misses", many_keys, queries}
    };
    std::vector<Case> cases;
    for (const auto& input : inputs)
        cases.push_back({input.name, [input](Seed seed) { check_searches(input.keys, input.queries, seed); }});
    return cases;
}

} // namespace tests
