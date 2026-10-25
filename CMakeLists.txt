cmake_minimum_required(VERSION 3.16)

project(skip_list LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(skip_list STATIC
    src/skip_list.cpp
)

target_include_directories(skip_list PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


add_library(skip_list_rebuild STATIC
    src/skip_list_rebuild.cpp
)

target_include_directories(skip_list_rebuild PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

