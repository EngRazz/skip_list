#include "test_support.hpp"
#include <numeric>

namespace tests {

static void check_union(const Keys& a_keys, const Keys& b_keys, Seed seed, bool different_heights) {
    Random random{seed, std::mt19937(seed)};
    List a(&random);
    OwnedList b(new List(&random));
    fill(a, a_keys);
    fill(*b, b_keys);
    if (different_heights) {
        auto height = [](List& list) {
            inspect(list);
            int levels = 0;
            for (Node* header = list.get_root(); header; header = header->next_d) ++levels;
            return levels;
        };
        for (int attempt = 0; attempt < 100 && height(a) == height(*b); ++attempt) {
            b.reset(new List(&random));
            fill(*b, b_keys);
        }
        require(height(a) != height(*b), "could not generate different input heights");
    }
    auto before_a = inspect(a), before_b = inspect(*b);
    KeySet expected(a_keys.begin(), a_keys.end());
    expected.insert(b_keys.begin(), b_keys.end());
    auto result = unite(a, *b);
    expect_contents(*result, expected);
    expect_preserved(before_a, inspect(*result));
    expect_preserved(before_b, inspect(*result));
    expect_reusable(a, {result.get(), b.get()});
    expect_reusable(*b, {result.get(), &a});
    for (Key key : {minimum, Key(7), maximum}) {
        bool should_insert = expected.insert(key).second;
        auto before = inspect(*result);
        require(implementation::insert(result.get(), key) == should_insert, "cannot insert into union output");
        expect_preserved(before, inspect(*result));
        expect_contents(*result, expected);
    }
    result.reset();
    expect_contents(a, {minimum, 42});
    expect_contents(*b, {minimum, 42});
}

static void check_union_lifetime(Seed seed) {
    Random random{seed, std::mt19937(seed)};
    OwnedList result;
    {
        List a(&random), b(&random);
        fill(a, {minimum, -1});
        fill(b, {1, maximum});
        result = unite(a, b);
    }
    expect_contents(*result, {minimum, -1, 1, maximum});
    expect_next_random_insert(*result, random, 0);
}

static void check_union_rng(Seed seed, bool empty_left) {
    Random random{seed, std::mt19937(seed)};
    OwnedList list(new List(&random));
    KeySet expected;
    for (Key key = 1; key <= 64; ++key) {
        expect_next_random_insert(*list, random, key);
        expected.insert(key);
        List empty(&random);
        auto result = empty_left ? unite(empty, *list) : unite(*list, empty);
        list = std::move(result);
        expect_contents(*list, expected);
    }
}

std::vector<Case> union_cases() {
    struct Input { std::string name; Keys a, b; bool different_heights = false; };
    Keys large(1000);
    std::iota(large.begin(), large.end(), 1);
    const std::vector<Input> inputs = {
        {"both inputs empty; reuse both", {}, {}},
        {"empty left; right contains INT64_MIN", {}, {minimum, 1, 2, 3}},
        {"empty right; left contains INT64_MIN", {minimum, -2, -1}, {}},
        {"two singletons", {-1}, {1}},
        {"disjoint ordered ranges", {-10, -4, 0}, {1, 8, 20}},
        {"small left, large right, different heights", {minimum}, large, true},
        {"large left, small right, different heights", large, {1001}, true},
        {"both int64 boundaries", {minimum, minimum + 1, -1}, {0, maximum}}
    };
    std::vector<Case> cases;
    for (const auto& input : inputs)
        cases.push_back({input.name, [input](Seed seed) { check_union(input.a, input.b, seed, input.different_heights); }});
    cases.push_back({"output survives destruction of consumed stack inputs", check_union_lifetime});
    cases.push_back({"64 unions with empty left: RNG sequence advances at every insertion", [](Seed seed) { check_union_rng(seed, true); }});
    cases.push_back({"64 unions with empty right: RNG sequence advances at every insertion", [](Seed seed) { check_union_rng(seed, false); }});
    return cases;
}

} // namespace tests
