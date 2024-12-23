#pragma once

#include <cstdint>
#include <string>
#include <string_view>
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

struct ALUInstruction {
    uint8_t reg_field;
};

extern std::unordered_map<std::string_view, ZOInstruction>   ZOTable;
extern std::unordered_map<std::string_view, ALUInstruction>  ALUTable;

bool check_forbidden_prefix(const Context& ctx, uint8_t p);
void assemble_zo(Context& ctx, const std::string_view& instruction, const std::string_view& args);
void assemble_alu(Context& ctx, const std::string_view& instruction, const std::string_view& args);