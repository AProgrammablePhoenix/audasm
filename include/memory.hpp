#pragma once

#include <cstdint>
#include <string>

#include "context.hpp"

struct MemoryOperandDescriptor {
    // Legacy 16-bit fields
    
    uint8_t size;   // 16, 32, or 64
    bool bx;        // true if BX is used
    bool bp;        // true if BP is used
    bool si;        // true if SI is used
    bool di;        // true if DI is used
    int64_t disp;   // displacement

    // Legacy 32-bit fields

    uint8_t index;  // SIB-index
    uint8_t scale;  // SIB-scale
    uint8_t base;   // SIB-base
};

struct MemoryOperand {
    uint8_t     size;
    uint8_t     modrm;
    bool        has_sib;
    uint8_t     sib;
    uint8_t     disp_size;
    uint64_t    disp;
};

bool parse_memory(Context& ctx, std::string rs, MemoryOperandDescriptor& mdesc);
uint8_t build_modrm_core(uint8_t rm, uint8_t reg, uint8_t mod);
bool make_modrm_sib(Context& ctx, MemoryOperandDescriptor desc, uint8_t reg_v, MemoryOperand& mop);

void output_disp_16(Context& ctx, uint8_t disp_size, uint64_t disp);
void output_disp_32(Context& ctx, uint8_t disp_size, uint64_t disp);