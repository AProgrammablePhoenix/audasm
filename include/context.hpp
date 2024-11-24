#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string_view>
#include <vector>

enum BitsMode {
    INVALID,
    M16,
    M32,
    M64
};

struct Context {
    BitsMode                b_mode;
    size_t                  line_no;
    std::ofstream           output_file;
    bool                    on_error;
    std::vector<uint8_t>    contextual_prefixes;
};

void change_bits_mode(Context& ctx, const std::string_view& s);
