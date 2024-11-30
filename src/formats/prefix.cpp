#include <algorithm>
#include <cstdint>

#include "context.hpp"

bool check_forbidden_prefix(const Context& ctx, uint8_t p) {
    return std::find(
        ctx.contextual_prefixes.cbegin(),
        ctx.contextual_prefixes.cend(),
        p
    ) != ctx.contextual_prefixes.cend();
}
