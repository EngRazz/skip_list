#pragma once

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct Options {
    std::string method;
    std::vector<std::uint32_t> seeds = {7, 17, 27};
    bool help = false;
};

inline std::vector<std::uint32_t> parse_seeds(const std::string& text) {
    const std::string error = "seeds must be comma-separated integers from 0 to 4294967295";
    if (text.empty() || text.back() == ',') throw std::invalid_argument(error);
    std::vector<std::uint32_t> seeds;
    std::istringstream input(text);
    std::string token;
    while (std::getline(input, token, ',')) {
        if (token.empty()) throw std::invalid_argument(error);
        std::uint64_t value = 0;
        for (char digit : token) {
            if (digit < '0' || digit > '9') throw std::invalid_argument(error);
            value = value * 10 + (digit - '0');
            if (value > std::numeric_limits<std::uint32_t>::max())
                throw std::invalid_argument(error);
        }
        auto seed = static_cast<std::uint32_t>(value);
        if (std::find(seeds.begin(), seeds.end(), seed) == seeds.end()) seeds.push_back(seed);
    }
    return seeds;
}

inline Options parse_options(int argc, char** argv, bool correctness) {
    Options options;
    if (argc == 2 && std::string(argv[1]) == "--help") {
        options.help = true;
        return options;
    }
    if (argc < 2) throw std::invalid_argument("choose a method or --all");
    options.method = argv[1];
    const std::vector<std::string> methods = {"--insert", "--search", "--ordered-union", "--split", "--split-rebuild", "--all"};
    if (std::find(methods.begin(), methods.end(), options.method) == methods.end())
        throw std::invalid_argument("unknown method: " + options.method);
    bool seeds_given = false;
    for (int i = 2; i < argc; ++i) {
        std::string argument = argv[i];
        if (argument == "--seeds" && !seeds_given && i + 1 < argc) {
            options.seeds = parse_seeds(argv[++i]);
            seeds_given = true;
        } else throw std::invalid_argument("unknown, repeated, or incomplete option: " + argument);
    }
    return options;
}

inline void print_usage(const char* program, bool correctness) {
    std::cout << "Usage: " << program << " <method> [--seeds 7,17,27]\n"
              << "Methods: --insert --search --ordered-union --split --split-rebuild --all\n"
              << "Seeds: one uint32 seed or a comma-separated list; default 7,17,27.\n"
              << (correctness ? "Correctness only: no timings or CSVs.\n"
                              : "Timings only: CSVs are written to benchmark/CSVs.\n");
}

inline void print_result(bool passed, const std::string& name) {
    std::cout << (passed ? "\033[32mPASS\033[0m" : "\033[31mFAIL\033[0m")
              << " - " << name << std::endl;
}
