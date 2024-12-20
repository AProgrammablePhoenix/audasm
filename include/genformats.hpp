#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "argument.hpp"
#include "context.hpp"
#include "memory.hpp"

struct FormatI {
    AsmRegister     reg;
    uint64_t        imm;
    uint8_t         op_imm_8;
    uint8_t         op_imm_def;
};

struct FormatRI {
    AsmRegister     reg;
    int32_t         reg_size;
    uint64_t        imm;

    uint8_t         default_reg_v;
    uint8_t         r8_imm8_op;
    uint8_t         r_def_imm8_op;
    uint8_t         r_imm_def_op;
};

struct FormatMI {
    MemoryOperandDescriptor mdesc;
    uint8_t                 size_override;
    uint64_t                imm;

    uint8_t                 default_reg_v;
    uint8_t                 r8_imm8_op;
    uint8_t                 r_imm_def_op;
    uint8_t                 r_def_imm8_op;
};

struct FormatRR {
    AsmRegister     reg_source;
    int32_t         reg_source_size;
    AsmRegister     reg_dest;
    int32_t         reg_dest_size;

    uint8_t         r8_op;
    uint8_t         r_def_op;
};

struct FormatMR {
    MemoryOperandDescriptor mdesc;
    uint8_t                 size_override;
    int32_t                 reg_size;

    uint8_t                 default_reg_v;
    uint8_t                 r8_rm8_op;
    uint8_t                 r_rm_def_op;

    std::vector<uint8_t>    prefixes;
    std::vector<uint8_t>    ex_prefixes;
};

bool x86_format_i(Context& ctx, const FormatI& fparams);
void x86_format_ri(Context& ctx, const std::string_view& instruction, const FormatRI& fparams);
void x86_format_mi(Context& ctx, const FormatMI& fparams);
void x86_format_rr(Context& ctx, const std::string_view& instruction, const FormatRR& fparams);
void x86_format_mr(Context& ctx, const FormatMR& fparams);
