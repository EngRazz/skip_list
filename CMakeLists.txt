cmake_minimum_required(VERSION 3.17)
project(skip_list LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Structural kernels and the rebuilding split share this implementation.
add_library(skip_list STATIC src/skip_list.cpp)
target_include_directories(skip_list PUBLIC include)

set(CORRECTNESS_SOURCES
    test/skip_list_test.cpp
    test/test_support.cpp
    test/insertion_tests.cpp
    test/search_tests.cpp
    test/union_tests.cpp
    test/split_tests.cpp
    test/mixed_tests.cpp)

add_executable(skip_list_test ${CORRECTNESS_SOURCES})
target_include_directories(skip_list_test PRIVATE support)
target_link_libraries(skip_list_test PRIVATE skip_list)

add_executable(skip_list_test_sanitized ${CORRECTNESS_SOURCES} src/skip_list.cpp)
target_include_directories(skip_list_test_sanitized PRIVATE include support)
target_compile_definitions(skip_list_test_sanitized PRIVATE TEST_WITH_ASAN)
target_compile_options(skip_list_test_sanitized PRIVATE
    -O2 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined)
target_link_options(skip_list_test_sanitized PRIVATE -fsanitize=address,undefined)

# Download the timing library only when benchmarks are requested.
option(BUILD_GOOGLE_BENCHMARK "Build the Google Benchmark driver" OFF)

if(BUILD_GOOGLE_BENCHMARK)
    include(FetchContent)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(google_benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        # Google Benchmark v1.10.0
        GIT_TAG a6b4a9f00b36c19af9997d68a1cf6e0b5b3d032c)
    FetchContent_MakeAvailable(google_benchmark)

    add_executable(skip_list_benchmark
        google_benchmark/src/main.cpp
        google_benchmark/src/search_benchmark.cpp
        google_benchmark/src/insertion_benchmark.cpp
        google_benchmark/src/union_benchmark.cpp
        google_benchmark/src/split_benchmark.cpp)
    target_link_libraries(skip_list_benchmark PRIVATE skip_list benchmark::benchmark)
endif()
