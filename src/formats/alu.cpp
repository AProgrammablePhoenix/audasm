#include <cstdint>
#include <format>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "context.hpp"
#include "formats.hpp"

#define ALU(v) \
    ALUInstruction { \
        .reg_field = v \
    } \

std::unordered_map<std::string, ALUInstruction> ALUTable = {
    { "ADC", ALU(2) },
    { "ADD", ALU(0) },
    { "AND", ALU(4) },
    { "CMP", ALU(7) },
    { "OR",  ALU(1) },
    { "SBB", ALU(3) },
    { "SUB", ALU(5) },
    { "XOR", ALU(6) }
};

void assemble_alu(Context& ctx, const std::string_view& instruction) {
    ALUInstruction alui = ALUTable.at(instruction.data());
}
