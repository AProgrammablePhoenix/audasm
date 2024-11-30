#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "context.hpp"

struct ZOInstruction {
    uint8_t opcode;
    std::vector<uint8_t> forbidden_prefixes;
    
    struct Mode {
        BitsMode mode;
        uint8_t prefix;
    } mode_prefix;

    std::vector<uint8_t> other_prefixes;
    bool hasOptionalImm8 = false;
};

extern std::unordered_map<std::string, ZOInstruction> ZOTable;

bool check_forbidden_prefix(const Context& ctx, uint8_t p);
void assemble_zo(Context& ctx, const std::string_view& instruction);
