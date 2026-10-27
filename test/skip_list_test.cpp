#include "test_support.hpp"

using namespace tests;


int main(int argc, char** argv) {
    try {
        auto options = parse_options(argc, argv, true);
        if (options.help) { print_usage(argv[0], true); return 0; }
        std::cout << "Implementation: " << implementation_name << "\nCorrectness checks only\nSeeds:";
        for (Seed seed : options.seeds) std::cout << ' ' << seed;
        std::cout << std::endl;

        Case initialization{"empty list and shared handler initialization", [](Seed seed) {
            Random random{seed, std::mt19937(seed)};
            auto generator = random.generator;
            {
                List list(&random);
                require(list.get_rh() == &random, "constructor lost the handler");
                expect_contents(list, {});
            }
            require(random.generator == generator, "constructing/destroying an empty list changed RNG state");
        }};
        if (!run_case(initialization, options.seeds.front())) {
            std::cout << "Fix list initialization before running the method checks.\n";
            return 1;
        }

        struct Suite {
            std::string flag, name;
            std::vector<Case> (*cases)();
        };
        const std::vector<Suite> suites = {
            {"--insert", "INSERTION", insertion_cases},
            {"--search", "SEARCH", search_cases},
            {"--ordered-union", "ORDERED UNION", union_cases},
            {"--split", "SPLIT", split_cases},
            {"--all", "MIXED OPERATIONS AND SHARED RNG", mixed_cases},
            {"--split-rebuild", "REBUILDING SPLIT", split_rebuild_cases},
            {"--split-rebuild", "REBUILDING SPLIT: MIXED OPERATIONS AND SHARED RNG", mixed_rebuild_cases}
        };
        std::vector<std::pair<std::string, bool>> results;
        int passed_count = 0, failed_count = 0;
        for (const auto& suite : suites) {
            if (options.method != "--all" && options.method != suite.flag) continue;
            std::cout << "\n========== " << suite.name << " ==========\n";
            bool passed = true;
            auto cases = suite.cases();
            for (Seed seed : options.seeds) {
                std::cout << "\nSeed " << seed << std::endl;
                for (const auto& test : cases) {
                    bool ok = run_case(test, seed);
                    if (ok) ++passed_count;
                    else { ++failed_count; passed = false; }
                }
            }
            results.push_back({suite.name, passed});
        }
        std::cout << "\n========== SUMMARY ==========\n";
        for (const auto& [name, passed] : results) print_result(passed, name);
        std::cout << passed_count << " passed, " << failed_count << " failed (plus initialization).\n";
        return failed_count ? 1 : 0;
    } catch (const std::exception& error) {
        print_result(false, error.what());
        print_usage(argv[0], true);
        return 1;
    }
}
