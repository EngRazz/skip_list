#include "test_support.hpp"

namespace tests {

static void check_split(const Keys& keys, Key pivot, Seed seed, bool rebuild) {
    Random random{seed, std::mt19937(seed)};
    List input(&random);
    fill(input, keys);
    auto before = inspect(input);
    KeySet original(keys.begin(), keys.end()), left_keys, right_keys;
    for (Key key : original) (key <= pivot ? left_keys : right_keys).insert(key);
    auto outputs = partition(input, pivot, rebuild);
    expect_contents(*outputs.left, left_keys);
    expect_contents(*outputs.right, right_keys);
    if (!rebuild) {
        auto combined = inspect(*outputs.left);
        auto right = inspect(*outputs.right);
        combined.towers.insert(right.towers.begin(), right.towers.end());
        require(combined.towers == before.towers, "split changed node addresses or heights");
    }
    expect_reusable(input, {outputs.left.get(), outputs.right.get()});
    auto restored = unite(*outputs.left, *outputs.right);
    expect_contents(*restored, original);
    if (!rebuild) require(inspect(*restored).towers == before.towers, "round trip changed towers");
    expect_contents(input, {minimum, 42});
}

static void check_independence(Seed seed, bool rebuild) {
    Random random{seed, std::mt19937(seed)};
    Partition outputs;
    {
        List input(&random);
        fill(input, {minimum, -8, -4, 0, 4, 8});
        outputs = partition(input, 0, rebuild);
        expect_reusable(input, {outputs.left.get(), outputs.right.get()});
    }
    auto before_right = inspect(*outputs.right);
    expect_next_random_insert(*outputs.left, random, -2);
    require(same_structure(before_right, inspect(*outputs.right)), "left insertion changed right output");
    auto before_left = inspect(*outputs.left);
    expect_next_random_insert(*outputs.right, random, 6);
    require(same_structure(before_left, inspect(*outputs.left)), "right insertion changed left output");
    expect_next_random_insert(*outputs.left, random, 3);
    require(!implementation::search(outputs.right.get(), 3), "outputs share inserted data");
    expect_next_random_insert(*outputs.right, random, 3);
    expect_contents(*outputs.left, {minimum, -8, -4, -2, 0, 3});
    expect_separate(*outputs.left, *outputs.right);
    outputs.left.reset();
    expect_contents(*outputs.right, {3, 4, 6, 8});
    expect_next_random_insert(*outputs.right, random, 10);
}

static void check_split_rng(Seed seed, bool keep_left, bool rebuild) {
    Random random{seed, std::mt19937(seed)};
    OwnedList list(new List(&random));
    KeySet expected;
    for (Key key = 1; key <= 64; ++key) {
        expect_next_random_insert(*list, random, key);
        expected.insert(key);
        auto outputs = partition(*list, keep_left ? maximum : minimum, rebuild);
        list = keep_left ? std::move(outputs.left) : std::move(outputs.right);
        expect_contents(*list, expected);
    }
}

static std::vector<Case> make_split_cases(bool rebuild) {
    struct Input { std::string name; Keys keys; Key pivot; };
    const Keys ordinary = {-10, -4, 0, 6, 20};
    const std::vector<Input> inputs = {
        {"empty list", {}, 0},
        {"empty list at INT64_MIN", {}, minimum},
        {"singleton: pivot below key", {5}, 4},
        {"singleton: pivot equals key", {5}, 5},
        {"singleton: pivot above key", {5}, 6},
        {"pivot before all keys", ordinary, -11},
        {"pivot after all keys", ordinary, 21},
        {"stored pivot belongs to left output", ordinary, 0},
        {"absent pivot between keys", ordinary, 3},
        {"stored INT64_MIN belongs to left output", {minimum, minimum + 1, 0, maximum}, minimum},
        {"absent INT64_MIN", {minimum + 1, maximum}, minimum},
        {"INT64_MIN singleton", {minimum}, minimum},
        {"split at zero preserves INT64_MIN", {minimum, minimum + 1, 0, maximum}, 0},
        {"INT64_MAX pivot", {minimum, maximum}, maximum}
    };
    std::vector<Case> cases;
    for (const auto& input : inputs)
        cases.push_back({input.name, [input, rebuild](Seed seed) { check_split(input.keys, input.pivot, seed, rebuild); }});
    cases.push_back({"outputs survive input destruction; independent updates and shared RNG", [rebuild](Seed seed) { check_independence(seed, rebuild); }});
    cases.push_back({"64 splits retaining left: shared RNG continues", [rebuild](Seed seed) { check_split_rng(seed, true, rebuild); }});
    cases.push_back({"64 splits retaining right: shared RNG continues", [rebuild](Seed seed) { check_split_rng(seed, false, rebuild); }});
    return cases;
}

std::vector<Case> split_cases() { return make_split_cases(false); }
std::vector<Case> split_rebuild_cases() { return make_split_cases(true); }

} // namespace tests
