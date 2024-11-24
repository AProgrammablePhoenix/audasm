#include <charconv>
#include <format>
#include <iostream>
#include <string_view>

#include "context.hpp"

void change_bits_mode(Context& ctx, const std::string_view& s) {
    unsigned int bits;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), bits);

    if (ec == std::errc{} && ptr == s.data() + s.size()) {
        if (bits == 16) {
            ctx.b_mode = BitsMode::M16;
            return;
        }
        else if (bits == 32) {
            ctx.b_mode = BitsMode::M32;
            return;
        }
        else if (bits == 64) {
            ctx.b_mode = BitsMode::M64;
            return;
        }
    }

    std::cerr << std::format(
        "Error on line {}: Invalid mode '{}' for BITS directive (accepted widths are 16, 32 and 64)",
        ctx.line_no,
        s
    ) << std::endl;
    ctx.on_error = true;
}
