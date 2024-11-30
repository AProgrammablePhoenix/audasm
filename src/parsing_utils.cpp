#include <charconv>
#include <format>
#include <iostream>
#include <string_view>
#include <vector>

#include "context.hpp"

namespace {
    static bool parse_number_base(const std::string_view& s, int base, uint64_t& res) {
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), res, base);
        return ec == std::errc{} && ptr == s.data() + s.size();
    }
}

std::string_view trim_string(const std::string_view& s) {
    size_t endpos   = s.find_last_not_of(" \t\n");
    size_t startpos = s.find_first_not_of(" \t");

    return (endpos == std::string_view::npos)
        ? ""
        : s.substr(startpos, endpos - startpos + 1);
    
}

std::vector<std::string_view> split_string(std::string_view s, char del) {
    std::vector<std::string_view> tokens;
    size_t i = 0;
    std::string_view tok;

    while ((i = s.find(del) != std::string_view::npos)) {
        tokens.emplace_back(s.substr(0, i));
        s = s.substr(i + 1);
    }
    tokens.push_back(s);

    return tokens;
}

bool parse_number(Context& ctx, const std::string_view& s, uint64_t& res) {
    if (s.starts_with("0x")) {
        const std::string_view& suffix = s.substr(2);
        if (!parse_number_base(suffix, 16, res)) {
            ctx.on_error = true;
            std::cerr << std::format(
                "Arithmetic error on line {}: invalid hexadecimal literal `{}`",
                ctx.line_no,
                s
            ) << std::endl;
        }
    }
    else if (s.starts_with("0o")) {
        const std::string_view& suffix = s.substr(2);
        if (!parse_number_base(suffix, 8, res)) {
            ctx.on_error = true;
            std::cerr << std::format(
                "Arithmetic error on line {}: invalid octal literal `{}`",
                ctx.line_no,
                s
            ) << std::endl;
        }
    }
    else if (s.starts_with("0b")) {
        const std::string_view& suffix = s.substr(2);
        if (!parse_number_base(suffix, 2, res)) {
            ctx.on_error = true;
            std::cerr << std::format(
                "Arithmetic error on line {}: invalid binary literal `{}`",
                ctx.line_no,
                s
            ) << std::endl;
        }
    }
    else {
        if (!parse_number_base(s, 10, res)) {
            ctx.on_error = true;
            std::cerr << std::format(
                "Arithmetic error on line {}: invalid decimal literal `{}`",
                ctx.line_no,
                s
            ) << std::endl;
        }
    }

    return !ctx.on_error;
}
