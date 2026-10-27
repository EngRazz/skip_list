#include "test_support.hpp"
#include <numeric>

namespace tests {

static void check_insertions(const Keys& keys, Seed seed) {
    Random random{seed, std::mt19937(seed)};
    List list(&random);
    KeySet expected;
    for (Key key : keys) {
        auto before = inspect(list);
        auto generator = random.generator;
        bool should_insert = expected.insert(key).second;
        require(implementation::insert(&list, key) == should_insert, "wrong insertion return value");
        auto after = inspect(list);
        require(after.keys == Keys(expected.begin(), expected.end()), "wrong contents after insertion");
        expect_preserved(before, after);
        if (!should_insert) {
            require(same_structure(before, after), "duplicate changed the structure");
            require(random.generator == generator, "duplicate advanced the generator");
        }
    }
    expect_contents(list, expected);
}

std::vector<Case> insertion_cases() {
    Keys increasing(1000);
    std::iota(increasing.begin(), increasing.end(), 1);
    Keys decreasing = increasing, shuffled = increasing;
    std::reverse(decreasing.begin(), decreasing.end());
    std::mt19937 workload(17);
    std::shuffle(shuffled.begin(), shuffled.end(), workload);
    const std::vector<std::pair<std::string, Keys>> inputs = {
        {"insert into an empty list", {5}},
        {"1000 increasing keys", increasing},
        {"1000 decreasing keys", decreasing},
        {"1000 shuffled keys", shuffled},
        {"duplicates preserve nodes, heights, and RNG state", {4, 2, 4, 1, 2, 1, 4}},
        {"negative keys and zero", {-5, 0, -2, 7, 0}},
        {"both int64 boundaries and adjacent keys", {minimum, maximum, minimum + 1, maximum - 1, minimum, maximum}},
        {"INT64_MIN inserted first; duplicate rejected", {minimum, minimum, 4, minimum}},
        {"INT64_MIN inserted after other keys", {4, 0, minimum, minimum + 1, minimum}}
    };
    std::vector<Case> cases;
    for (const auto& input : inputs)
        cases.push_back({input.first, [keys = input.second](Seed seed) { check_insertions(keys, seed); }});
    return cases;
}

} // namespace tests
