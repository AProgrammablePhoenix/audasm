#pragma once

#include <cstdint>
#include <limits>
#include <string_view>
#include <type_traits>
#include <vector>

#include "argument.hpp"
#include "context.hpp"

std::string_view trim_string(const std::string_view& s);
std::vector<std::string_view> split_string(std::string_view s, char del);
bool parse_number(Context& ctx, const std::string_view& s, uint64_t& res);
std::vector<AsmArg> expect_arguments(Context& ctx, const std::string_view& s, size_t N);

template<typename T> requires std::numeric_limits<T>::is_integer bool test_number(int64_t n) {
    return
        (n >= (int64_t)std::numeric_limits<T>::min() && n <= (int64_t)std::numeric_limits<T>::max())
        || ((uint64_t)n >= 0 && (uint64_t)n <= (uint64_t)std::numeric_limits<typename std::make_unsigned<T>::type>::max());
}

template<typename T> requires std::numeric_limits<T>::is_integer bool test_number_strict(int64_t n) {
    return n >= (int64_t)std::numeric_limits<T>::min() && n <= (int64_t)std::numeric_limits<T>::max();
}
