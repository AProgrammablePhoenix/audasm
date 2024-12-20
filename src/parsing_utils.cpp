#include <charconv>
#include <format>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

#include "argument.hpp"
#include "context.hpp"
#include "memory.hpp"
#include "registers.hpp"

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

std::vector<AsmArg> expect_arguments(Context& ctx, const std::string_view& s, size_t N) {
    std::string_view normalized = trim_string(s);
    
    size_t comment_pos = normalized.find_first_of(';');
    if (comment_pos != std::string_view::npos) {
        normalized = normalized.substr(0, comment_pos);
    }

    std::vector<std::string_view> args = split_string(normalized, ',');
    if (args.size() != N) {
        return {};
    }

    std::vector<AsmArg> parsed_args;

    for (const auto& arg : args) {
        std::string_view trimmed_arg = trim_string(arg);
        uint8_t size_override = 0;

        if (trimmed_arg.starts_with("%byte")) {
            constexpr size_t prefix_length = 5;
            size_override = 8;
            trimmed_arg = trim_string(trimmed_arg.substr(prefix_length));
        }
        else if (trimmed_arg.starts_with("%word")) {
            constexpr size_t prefix_length = 5;
            size_override = 16;
            trimmed_arg = trim_string(trimmed_arg.substr(prefix_length));
        }
        else if (trimmed_arg.starts_with("%dword")) {
            constexpr size_t prefix_length = 6;
            size_override = 32;
            trimmed_arg = trim_string(trimmed_arg.substr(prefix_length));
        }
        else if (trimmed_arg.starts_with("%qword")) {
            constexpr size_t prefix_length = 6;
            size_override = 64;
            trimmed_arg = trim_string(trimmed_arg.substr(prefix_length));
        }

        if (REGISTERS.contains(trimmed_arg.data())) {
            if (size_override != 0) {
                std::cerr << std::format(
                    "Error on line {}: Did not expect a size prefix before a register",
                    ctx.line_no
                ) << std::endl;
                ctx.on_error = true;
                return {};
            }

            parsed_args.emplace_back(AsmArg {
                .type = AsmArgType::REGISTER,
                .reg = REGISTERS.at(trimmed_arg.data())
            });
        }
        else if (trimmed_arg.starts_with('[')) {
            if (trimmed_arg.ends_with(']')) {
                std::string_view memop = trimmed_arg.substr(1, trimmed_arg.size() - 2);
                /// TODO: parse memory operand
                MemoryOperandDescriptor mdesc;
                if (!parse_memory(ctx, memop.data(), mdesc)) {
                    std::cout << std::format(
                        "Error on line {}: Invalid memory operand detected for `{}`",
                        ctx.line_no,
                        trimmed_arg
                    ) << std::endl;
                    ctx.on_error = true;
                    return {};
                }

                parsed_args.emplace_back(AsmArg {
                    .type = AsmArgType::MEMORY,
                    .mem = { mdesc, size_override }
                });
            }
            else {
                std::cerr << std::format(
                    "Error on line {}: Did not expect '[' in '{}' (found in '{}')",
                    ctx.line_no,
                    trimmed_arg,
                    s
                );
                ctx.on_error = true;
                return {};
            }
        }
        else {
            if (size_override != 0) {
                std::cerr << std::format(
                    "Error on line {}: Did not expect a size prefix before an immediate",
                    ctx.line_no
                ) << std::endl;
                ctx.on_error = true;
                return {};
            }
            
            uint64_t imm;
            if (!parse_number(ctx, trimmed_arg, imm)) {
                std::cerr << std::format(
                    "Error on line {}: Invalid argument format for `{}`",
                    ctx.line_no,
                    trimmed_arg
                ) << std::endl;
                ctx.on_error = true;
                return {};
            }                
            else {
                parsed_args.emplace_back(AsmArg {
                    .type = AsmArgType::IMMEDIATE,
                    .imm = imm
                });
            }
        }
    }

    return parsed_args;
}