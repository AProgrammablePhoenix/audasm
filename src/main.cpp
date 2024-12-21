#include <cstdint>
#include <cstddef>

#include <algorithm>
#include <charconv>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "argument.hpp"
#include "context.hpp"
#include "formats.hpp"
#include "memory.hpp"
#include "parsing_utils.hpp"
#include "registers.hpp"

namespace {
    static void assemble_line(Context& ctx, const std::string_view& s) {
        if (s.empty() || s.starts_with("//") || s.starts_with(";") || s.starts_with("#")) {
            return;
        }
        else if (s.starts_with("BITS ")) {
            constexpr size_t prefix_length = 5;
            change_bits_mode(ctx, s.substr(prefix_length));
        }
        else if (s.starts_with("[BITS ")) {
            constexpr size_t prefix_length = 6;
            change_bits_mode(ctx, s.substr(prefix_length, s.size() - prefix_length - 1));
        }
        else {
            size_t delimiter_pos = s.find(" ");
            std::string_view instruction = s.substr(0, delimiter_pos);
            std::string_view args = delimiter_pos != std::string_view::npos ? s.substr(delimiter_pos + 1) : "";

            if (ZOTable.contains(instruction)) {
                assemble_zo(ctx, instruction, args);
            }
            else if (ALUTable.contains(instruction)) {
                assemble_alu(ctx, instruction, args);
            }
            else {
                std::cerr << std::format(
                    "Error on line {}: Unknown instruction `{}`",
                    ctx.line_no,
                    instruction.data()
                ) << std::endl;
                ctx.on_error = true;
                return;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: aus <input file> <output file>" << std::endl;
        return -1;
    }

    const char* input_file_path     = argv[1];
    const char* output_file_path    = argv[2];

    std::ifstream input_file(input_file_path);
    if (!input_file) {
        std::cerr << "Error: Could not open " << input_file_path << std::endl;
        return -1;
    }

    std::ofstream output_file(output_file_path, std::ios::binary);
    if (!output_file) {
        std::cerr << "Error: could not open " << output_file_path << std::endl;
        return -1;
    }

    Context ctx = {
        .b_mode         = M16,
        .line_no        = 1,
        .output_file    = std::move(output_file),
        .on_error       = false
    };

    std::string line;
    while (std::getline(input_file, line)) {
        size_t endpos   = line.find_last_not_of(" \t\n");
        size_t startpos = line.find_first_not_of(" \t");

        if (endpos == std::string::npos) {
            ++ctx.line_no;
            continue;
        }

        std::string normalized = line.substr(startpos, endpos - startpos + 1);

        std::transform(
            normalized.begin(),
            normalized.end(),
            normalized.begin(),
            [](unsigned char c) { return std::toupper(c); }
        );

        assemble_line(ctx, normalized);
        ++ctx.line_no;
    }

    if (ctx.on_error) {
        std::cerr << "Generation failed, output file may contain invalid data" << std::endl;
        return -1;
    }

    return 0;
}
