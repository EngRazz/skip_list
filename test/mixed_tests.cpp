#include "test_support.hpp"

namespace tests {

static void insert_and_compare(List& list, Random& random, KeySet& expected, Key key) {
    if (expected.insert(key).second) {
        expect_next_random_insert(list, random, key);
    } else {
        auto before = inspect(list);
        auto generator = random.generator;
        require(!implementation::insert(&list, key), "duplicate accepted");
        require(same_structure(before, inspect(list)), "duplicate changed nodes");
        require(random.generator == generator, "duplicate changed RNG state");
    }
    expect_contents(list, expected);
}

static void check_mixed(Seed seed, bool rebuild) {
    Random random{seed, std::mt19937(seed)};
    std::mt19937 workload(seed ^ 0x9e3779b9u);
    std::uniform_int_distribution<Key> key_dist(-200, 200);
    OwnedList list(new List(&random));
    fill(*list, {minimum, maximum});
    KeySet expected = {minimum, maximum};

    for (int step = 0; step < 1000; ++step) {
        try {
            auto before = inspect(*list);
            Key key = step % 31 == 0 ? minimum : step % 37 == 0 ? maximum : key_dist(workload);
            switch (step % 4) {
            case 0:
                insert_and_compare(*list, random, expected, key);
                break;
            case 1: {
                auto generator = random.generator;
                Node* found = implementation::search(list.get(), key);
                require((found != nullptr) == (expected.count(key) != 0), "wrong mixed search result");
                require(!found || found->k == key, "search returned a different key");
                require(same_structure(before, inspect(*list)), "search changed nodes");
                require(random.generator == generator, "search changed RNG state");
                break;
            }
            case 2: {
                auto outputs = partition(*list, key, rebuild);
                KeySet left, right;
                for (Key value : expected) (value <= key ? left : right).insert(value);
                expect_contents(*outputs.left, left);
                expect_contents(*outputs.right, right);
                expect_reusable(*list, {outputs.left.get(), outputs.right.get()});
                auto right_before = inspect(*outputs.right);
                insert_and_compare(*outputs.left, random, left, key);
                require(same_structure(right_before, inspect(*outputs.right)), "left insertion changed right output");
                expected.insert(key);
                auto left_before = inspect(*outputs.left);
                if (key != maximum) {
                    insert_and_compare(*outputs.right, random, right, key + 1);
                    expected.insert(key + 1);
                }
                require(same_structure(left_before, inspect(*outputs.left)), "right insertion changed left output");
                auto result = unite(*outputs.left, *outputs.right);
                list = std::move(result);
                break;
            }
            case 3: {
                List empty(&random);
                auto result = step % 8 == 3 ? unite(*list, empty) : unite(empty, *list);
                expect_reusable(empty, {result.get(), list.get()});
                list = std::move(result);
                break;
            }
            }
            expect_contents(*list, expected);
            if (!rebuild || step % 4 != 2) expect_preserved(before, inspect(*list));
        } catch (const std::exception& error) {
            throw std::runtime_error("mixed step " + std::to_string(step + 1) + ": " + error.what());
        }
    }
}

static void check_shared_sequence(Seed seed, bool rebuild) {
    Random random{seed, std::mt19937(seed)};
    List input(&random);
    fill(input, {-1, 1});
    auto outputs = partition(input, 0, rebuild);
    KeySet left = {-1}, right = {1}, reused;
    for (Key i = 1; i <= 32; ++i) {
        insert_and_compare(*outputs.left, random, left, -10 - i);
        insert_and_compare(*outputs.right, random, right, 10 + i);
        insert_and_compare(input, random, reused, i);
        expect_contents(*outputs.left, left);
        expect_contents(*outputs.right, right);
        expect_separate(input, *outputs.left);
        expect_separate(input, *outputs.right);
    }
    outputs.left.reset();
    outputs.right.reset();
    expect_next_random_insert(input, random, maximum);
}

std::vector<Case> mixed_cases() {
    return {
        {"1000 mixed steps against std::set, including consumed-input reuse", [](Seed seed) { check_mixed(seed, false); }},
        {"interleaved inserts into both outputs and reused input follow one RNG stream", [](Seed seed) { check_shared_sequence(seed, false); }},
        {"owning list disables copy construction", [](Seed) {
            require(!std::is_copy_constructible_v<List>, "copy construction is enabled for an owning list");
        }},
        {"owning list disables copy assignment", [](Seed) {
            require(!std::is_copy_assignable_v<List>, "copy assignment is enabled for an owning list");
        }}
    };
}

std::vector<Case> mixed_rebuild_cases() {
    return {
        {"1000 mixed steps using rebuilding split and structural union", [](Seed seed) { check_mixed(seed, true); }},
        {"rebuilt outputs and reused input follow one RNG stream", [](Seed seed) { check_shared_sequence(seed, true); }}
    };
}

} // namespace tests
