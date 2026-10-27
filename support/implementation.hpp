#pragma once

#include "skip_list.hpp"
namespace implementation = skip_list_kernels;
inline constexpr const char* implementation_name = "skip_list.cpp";

using List = implementation::skip_list;
using Node = implementation::node_t;
using Random = implementation::random_handler;
