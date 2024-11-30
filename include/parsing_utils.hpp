#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "context.hpp"

std::string_view trim_string(const std::string_view& s);
std::vector<std::string_view> split_string(std::string_view s, char del);
bool parse_number(Context& ctx, const std::string_view& s, uint64_t& res);
