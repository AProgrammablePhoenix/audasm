#include <cstdint>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "argument.hpp"
#include "context.hpp"
#include "formats.hpp"
#include "genformats.hpp"
#include "parsing_utils.hpp"

#define ALU(v) \
    ALUInstruction { \
        .reg_field = v \
    } \

std::unordered_map<std::string_view, ALUInstruction> ALUTable = {
    { "ADC", ALU(2) },
    { "ADD", ALU(0) },
    { "AND", ALU(4) },
    { "CMP", ALU(7) },
    { "OR",  ALU(1) },
    { "SBB", ALU(3) },
    { "SUB", ALU(5) },
    { "XOR", ALU(6) }
};

void assemble_alu(Context& ctx, const std::string_view& instruction, const std::string_view& args) {
    ALUInstruction alui = ALUTable.at(instruction);

    const uint8_t opcode_imm_8      = 0x04 + 0x08 * alui.reg_field;
    const uint8_t opcode_imm_def    = 0x05 + 0x08 * alui.reg_field;
    const uint8_t opcode_rm8_imm8   = 0x80;
    const uint8_t opcode_rm_imm     = 0x81;
    const uint8_t opcode_rm_imm8    = 0x83;
    const uint8_t opcode_rm8_r8     = 0x00 + 0x08 * alui.reg_field;
    const uint8_t opcode_rm_r       = 0x01 + 0x08 * alui.reg_field;
    const uint8_t opcode_r8_rm8     = 0x02 + 0x08 * alui.reg_field;
    const uint8_t opcode_r_rm       = 0x03 + 0x08 * alui.reg_field;

    const std::vector<AsmArg> parsed_args = expect_arguments(ctx, args, 2);
    if (parsed_args.empty() || ctx.on_error) {
        std::cerr << std::format(
            "Error on line {}: Invalid number of arguments for `{}`: `{}`",
            ctx.line_no,
            instruction,
            args
        ) << std::endl;
        ctx.on_error = true;
        return;
    }

    if (parsed_args[1].type == AsmArgType::IMMEDIATE) {
        const auto& imm = parsed_args[1].imm;

        if (parsed_args[0].type == AsmArgType::REGISTER) {
            const auto& [r0, s0] = parsed_args[0].reg;

            if (!x86_format_i(ctx, FormatI {
                .reg        = r0,
                .imm        = imm,
                .op_imm_8   = opcode_imm_8,
                .op_imm_def = opcode_imm_def
            })) {
                return x86_format_ri(ctx, instruction, FormatRI {
                    .reg            = r0,
                    .reg_size       = s0,
                    .imm            = imm,
                    .default_reg_v  = alui.reg_field,
                    .r8_imm8_op     = opcode_rm8_imm8,
                    .r_def_imm8_op  = opcode_rm_imm8,
                    .r_imm_def_op   = opcode_rm_imm
                });
            }
        }
        else if (parsed_args[0].type == AsmArgType::MEMORY) {
            const auto& [mdesc, size_override] = parsed_args[0].mem;
            return x86_format_mi(ctx, FormatMI {
                .mdesc          = mdesc,
                .size_override  = size_override,
                .imm            = imm,
                .default_reg_v  = alui.reg_field,
                .r8_imm8_op     = opcode_rm8_imm8,
                .r_imm_def_op   = opcode_rm_imm,
                .r_def_imm8_op  = opcode_rm_imm8
            });
        }
        else {
            std::cerr << std::format(
                "Error on line {}: Wrong destination operand type for `{}`, expected a register of memory operand",
                ctx.line_no,
                instruction
            ) << std::endl;
            ctx.on_error = true;
            return;
        }
    }
    else  if (parsed_args[1].type == AsmArgType::REGISTER) {
        const auto& [rs, ss] = parsed_args[1].reg;

        if (parsed_args[0].type == AsmArgType::REGISTER) {
            const auto& [rd, sd] = parsed_args[0].reg;
            return x86_format_rr(ctx, instruction, FormatRR {
                .reg_source         = rs,
                .reg_source_size    = ss,
                .reg_dest           = rd,
                .reg_dest_size      = sd,
                .r8_op              = opcode_rm8_r8,
                .r_def_op           = opcode_rm_r
            });
        }
        else if (parsed_args[0].type == AsmArgType::MEMORY) {
            const auto& [mdesc, size_override] = parsed_args[0].mem;
            return x86_format_mr(ctx, FormatMR {
                .mdesc          = mdesc,
                .size_override  = size_override,
                .reg_size       = ss,
                .default_reg_v  = REGISTERS_ENCODING.at(rs),
                .r8_rm8_op      = opcode_rm8_r8,
                .r_rm_def_op    = opcode_rm_r,
                .prefixes       = {},
                .ex_prefixes    = {}
            });
        }
        else {
            std::cerr << std::format(
                "Error on line {}: Wrong destination operand type for `{}`, expected a register or memory operand",
                ctx.line_no,
                instruction
            ) << std::endl;
            ctx.on_error = true;
            return;
        }
    }
    else if (parsed_args[1].type == AsmArgType::MEMORY) {
        const auto& [mdesc, size_override] = parsed_args[1].mem;

        if (parsed_args[0].type == AsmArgType::REGISTER) {
            const auto& [rd, sd] = parsed_args[0].reg;
            return x86_format_mr(ctx, FormatMR {
                .mdesc          = mdesc,
                .size_override  = size_override,
                .reg_size       = sd,
                .default_reg_v  = REGISTERS_ENCODING.at(rd),
                .r8_rm8_op      = opcode_r8_rm8,
                .r_rm_def_op    = opcode_r_rm,
                .prefixes       = {},
                .ex_prefixes    = {}
            });
        }
        else {
            std::cerr << std::format(
                "Error on line {}: Wrong destination operand type for `{}`, expected a register or memory operand",
                ctx.line_no,
                instruction
            ) << std::endl;
            ctx.on_error = true;
            return;
        }
    }
}
