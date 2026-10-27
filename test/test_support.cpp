#include "test_support.hpp"

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <unordered_set>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef TEST_WITH_ASAN
#include <sanitizer/asan_interface.h>
#endif

namespace tests {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

Snapshot inspect(List& list) {
    Snapshot result{list.get_root(), {}, {}, {}};
    require(result.root, "null root");
    std::vector<std::map<Key, Node*>> levels;
    std::unordered_set<Node*> visited;
    for (Node* header = result.root; header; header = header->next_d) {
        require(visited.insert(header).second, "cycle or shared header");
        require(header->k == minimum, "wrong sentinel key");
        result.links[header] = {header->k, header->next_r, header->next_d};
        std::map<Key, Node*> level;
        for (Node* node = header->next_r; node; node = node->next_r) {
            require(visited.insert(node).second, "cycle or shared node");
            require(level.empty() || node->k > level.rbegin()->first, "keys are not strictly increasing");
            level[node->k] = node;
            result.towers[node->k].push_back(node);
            result.links[node] = {node->k, node->next_r, node->next_d};
        }
        levels.push_back(std::move(level));
    }
    for (std::size_t i = 0; i < levels.size(); ++i) {
        for (auto [key, node] : levels[i]) {
            Node* below = nullptr;
            if (i + 1 < levels.size()) {
                auto found = levels[i + 1].find(key);
                require(found != levels[i + 1].end(), "upper key absent from the next level");
                below = found->second;
            }
            require(node->next_d == below, "incorrect downward link");
        }
    }
    for (auto [key, node] : levels.back()) result.keys.push_back(key);
    return result;
}

bool same_structure(const Snapshot& before, const Snapshot& after) {
    return before.root == after.root && before.links == after.links;
}

void expect_contents(List& list, const KeySet& expected) {
    auto before = inspect(list);
    require(before.keys == Keys(expected.begin(), expected.end()), "contents differ from std::set");
    auto generator = list.get_rh()->generator;
    for (Key key : expected) {
        Node* found = implementation::search(&list, key);
        const auto& tower = before.towers.at(key);
        require(std::find(tower.begin(), tower.end(), found) != tower.end(), "search did not return a data node");
    }
    require(same_structure(before, inspect(list)), "search changed the structure");
    require(list.get_rh()->generator == generator, "search advanced the generator");
}

void expect_separate(List& a, List& b) {
    auto left = inspect(a), right = inspect(b);
    for (const auto& entry : left.links)
        require(!right.links.count(entry.first), "lists share an owned node or sentinel");
}

void expect_preserved(const Snapshot& before, const Snapshot& after) {
    for (const auto& [key, nodes] : before.towers) {
        auto found = after.towers.find(key);
        require(found != after.towers.end() && found->second == nodes, "existing node address or height changed");
    }
}

void expect_reusable(List& input, std::initializer_list<List*> outputs) {
    expect_contents(input, {});
    require(!implementation::search(&input, minimum), "empty input matches its sentinel");
    std::vector<Snapshot> before;
    for (List* output : outputs) {
        expect_separate(input, *output);
        before.push_back(inspect(*output));
    }
    require(implementation::insert(&input, minimum), "cannot reuse input for INT64_MIN");
    require(implementation::insert(&input, 42), "cannot reuse input for another key");
    expect_contents(input, {minimum, 42});
    std::size_t i = 0;
    for (List* output : outputs) {
        require(same_structure(before[i++], inspect(*output)), "reusing input changed an output");
        expect_separate(input, *output);
    }
}

void fill(List& list, const Keys& keys) {
    for (Key key : keys) implementation::insert(&list, key);
}

// Every old node must remain reachable or have been freed. ASan marks freed
// allocations as poisoned, so this also catches detached, leaked sentinels.
static void expect_accounted_for(const Snapshot& before, std::initializer_list<List*> lists) {
#ifdef TEST_WITH_ASAN
    std::unordered_set<Node*> reachable;
    for (List* list : lists)
        for (const auto& entry : inspect(*list).links) reachable.insert(entry.first);
    for (const auto& entry : before.links) {
        if (!reachable.count(entry.first))
            require(__asan_address_is_poisoned(entry.first), "old node/sentinel was detached without being freed");
    }
#else
    (void)before;
    (void)lists;
#endif
}

OwnedList unite(List& a, List& b) {
    Random* random = a.get_rh();
    require(random && random == b.get_rh(), "union operands need the same handler");
    auto generator = random->generator;
    auto before_a = inspect(a), before_b = inspect(b);
    OwnedList result(implementation::ordered_union(&a, &b));
    require(result && result->get_rh() == random, "union lost the shared handler");
    expect_contents(a, {});
    expect_contents(b, {});
    expect_separate(a, b);
    expect_separate(a, *result);
    expect_separate(b, *result);
    expect_accounted_for(before_a, {&a, &b, result.get()});
    expect_accounted_for(before_b, {&a, &b, result.get()});
    require(random->generator == generator, "structural union changed the generator");
    return result;
}

Partition partition(List& input, Key pivot, bool rebuild) {
    Random* random = input.get_rh();
    auto generator = random->generator;
    auto before = inspect(input);
    std::map<Key, std::size_t> expected_heights;
    if (rebuild) {
        auto promote = random->promote;
        for (Key key : before.keys) {
            std::size_t height = 1;
            while (promote(generator)) ++height;
            expected_heights[key] = height;
        }
    }
    auto [left, right] = rebuild ? implementation::split_rebuild(&input, pivot)
                                : implementation::split(&input, pivot);
    Partition result{OwnedList(left), OwnedList(right)};
    require(left && right && left != right, "split must return two distinct lists");
    require(left->get_rh() == random && right->get_rh() == random, "split lost the shared handler");
    expect_contents(input, {});
    expect_separate(input, *left);
    expect_separate(input, *right);
    expect_separate(*left, *right);
    expect_accounted_for(before, {&input, left, right});
    if (rebuild) {
        require(random->generator == generator,
                "rebuilding split must draw fresh heights: check the split_rebuild wrapper");
        auto rebuilt = inspect(*left);
        auto right_nodes = inspect(*right);
        rebuilt.towers.insert(right_nodes.towers.begin(), right_nodes.towers.end());
        require(rebuilt.towers.size() == expected_heights.size(), "rebuild lost or added keys");
        for (auto [key, height] : expected_heights) {
            require(rebuilt.towers.at(key).size() == height, "rebuild did not use the next ordinary insertion height");
            for (Node* node : rebuilt.towers.at(key))
                require(!before.links.count(node), "rebuild reused an original node instead of allocating a new one");
        }
    } else {
        require(random->generator == generator, "structural split changed the generator");
    }
    return result;
}

// Predict one geometric height using a COPY of the shared handler state.
// The real insertion must consume precisely that next sequence of promotions.
void expect_next_random_insert(List& list, Random& random, Key key) {
    require(list.get_rh() == &random, "wrong random handler");
    auto expected_generator = random.generator;
    auto promote = random.promote;
    std::size_t expected_height = 1;
    while (promote(expected_generator)) ++expected_height;
    require(implementation::insert(&list, key), "new key was rejected");
    require(random.generator == expected_generator, "insertion restarted or skipped the shared random sequence");
    require(inspect(list).towers.at(key).size() == expected_height, "height differs from the next shared random draws");
}

bool run_case(const Case& test, Seed seed) {
    std::cout.flush();
    std::cerr.flush();
    pid_t child = fork();
    require(child >= 0, "cannot start an isolated check");
    if (child == 0) {
        rlimit limit{0, 0};
        setrlimit(RLIMIT_CORE, &limit);
        alarm(90);
        try {
            test.check(seed);
            std::exit(0); // Run sanitizer exit hooks after local objects are destroyed.
        } catch (const std::exception& error) {
            std::cout << "  " << error.what() << std::endl;
            std::exit(1);
        }
    }
    int status = 0;
    while (waitpid(child, &status, 0) < 0)
        require(errno == EINTR, "cannot wait for an isolated check");
    if (WIFSIGNALED(status))
        std::cout << "  Child stopped by signal " << WTERMSIG(status) << " (crash or timeout).\n";
    bool passed = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    print_result(passed, test.name);
    return passed;
}

} // namespace tests
