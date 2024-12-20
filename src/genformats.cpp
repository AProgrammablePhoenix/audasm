#include <cstdint>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

#include "argument.hpp"
#include "context.hpp"
#include "genformats.hpp"
#include "memory.hpp"
#include "parsing_utils.hpp"

bool x86_format_i(Context& ctx, const FormatI& fparams) {
    if (fparams.reg == AsmRegister::AL && test_number<int8_t>(fparams.imm)) {
        ctx.output_file.put(fparams.op_imm_8);
        ctx.output_file.write((const char*)&fparams.imm, sizeof(int8_t));
        return true;
    }
    else if (fparams.reg == AsmRegister::AX && test_number<int16_t>(fparams.imm)) {
        if (ctx.b_mode == BitsMode::M32 || ctx.b_mode == BitsMode::M64) {
            ctx.output_file.put(0x66);
        }
        ctx.output_file.put(fparams.op_imm_def);
        ctx.output_file.write((const char*)&fparams.imm, sizeof(int16_t));
        return true;
    }
    else if (fparams.reg == AsmRegister::EAX && test_number<int32_t>(fparams.imm)) {
        if (ctx.b_mode == BitsMode::M16) {
            ctx.output_file.put(0x66);
        }
        ctx.output_file.put(fparams.op_imm_def);
        ctx.output_file.write((const char*)&fparams.imm, sizeof(int32_t));
        return true;
    }

    return false;
}

void x86_format_ri(Context& ctx, const std::string_view& instruction, const FormatRI& fparams) {
    const uint8_t modrm = build_modrm_core(REGISTERS_ENCODING.at(fparams.reg), fparams.default_reg_v, 0b11);
    const auto& imm = fparams.imm;

    switch (fparams.reg_size) {
        case 8: {
            if (test_number<int8_t>(imm)) {
                std::cerr << std::format(
                    "Warning on line {}: Immediate value `{}` too large to fit within 8 bits, truncating to 8 bits",
                    ctx.line_no,
                    imm
                ) << std::endl;
            }

            ctx.output_file.put(fparams.r8_imm8_op);
            ctx.output_file.put(modrm);
            ctx.output_file.put((uint8_t)imm);
            return;
        }
        case 16: {
            if (ctx.b_mode == BitsMode::M32 ||ctx.b_mode == BitsMode::M64) {
                ctx.output_file.put(0x66);
            }

            if (test_number<int8_t>(imm)) {
                ctx.output_file.put(fparams.r_def_imm8_op);
                ctx.output_file.put(modrm);
                ctx.output_file.put((uint8_t)imm);
            }
            else {
                if (!test_number<int16_t>(imm)) {
                    std::cerr << std::format(
                        "Warning on line {}: Immediate value `{}` too large to fit within 16 bits, truncating to 16 bits",
                        ctx.line_no,
                        imm
                    ) << std::endl;
                }

                ctx.output_file.put(fparams.r_imm_def_op);
                ctx.output_file.put(modrm);
                ctx.output_file.write((const char*)&imm, sizeof(int16_t));
            }
            return;
        }
        case 32: {
            if (ctx.b_mode == BitsMode::M16) {
                ctx.output_file.put(0x66);
            }

            if (test_number<int8_t>(imm)) {
                ctx.output_file.put(fparams.r_def_imm8_op);
                ctx.output_file.put(modrm);
                ctx.output_file.put((uint8_t)imm);
            }
            else {
                if (!test_number<int32_t>(imm)) {
                    std::cerr << std::format(
                        "Warning on line {}: Immediate value `{}` too large to fit within 32 bits, truncating to 32 bits",
                        ctx.line_no,
                        imm
                    ) << std::endl;
                }

                ctx.output_file.put(fparams.r_imm_def_op);
                ctx.output_file.put(modrm);
                ctx.output_file.write((const char*)&imm, sizeof(int32_t));
            }
            return;
        }
        default: {
            std::cerr << std::format(
                "Error on line {}: Invalid register used as argument for `{}`",
                ctx.line_no,
                instruction
            ) << std::endl;
            ctx.on_error = true;
            return;
        }
    }
}

template<size_t SIZE> static void print_size_warning(const Context& ctx, uint64_t imm) {
    switch (SIZE) {
        case 8: {
            if (!test_number<int8_t>(imm)) {
                std::cerr << std::format(
                    "Warning on line {}: Immediate value too large to fit in 8 bits, truncating to 8 bits",
                    ctx.line_no
                ) << std::endl;
            }
            break;
        }
        case 16: {
            if (!test_number<int16_t>(imm)) {
                std::cerr << std::format(
                    "Warning on line {}: Immediate value too large to fit in 16 bits, truncating to 16 bits",
                    ctx.line_no
                ) << std::endl;
            }
            break;
        }
        case 32: {
            if (!test_number<int32_t>(imm)) {
                std::cerr << std::format(
                    "Warning on line {}: Immediate value too large to fit in 32 bits, truncating to 32 bits",
                    ctx.line_no
                ) << std::endl;
            }
            break;
        }
        default: break;
    }
}

template<size_t IMM_SIZE, uint8_t DISP_MODE> static void generate_mi(
    Context& ctx,
    const std::vector<uint8_t> prefixes,
    uint8_t op,
    const MemoryOperand& mmop,
    uint64_t imm
) {
    ctx.output_file.write((const char*)prefixes.data(), prefixes.size());
    ctx.output_file.put(op);
    ctx.output_file.put(mmop.modrm);

    if (mmop.has_sib) {
        ctx.output_file.put(mmop.sib);
    }

    if (DISP_MODE == 16) {
        output_disp_16(ctx, mmop.disp_size, mmop.disp);
    }
    else  {
        output_disp_32(ctx, mmop.disp_size, mmop.disp);
    }

    switch (IMM_SIZE) {
        case  8: ctx.output_file.put((uint8_t)imm); break;
        case 16: ctx.output_file.write((const char*)imm, sizeof(uint16_t)); break;
        case 32: ctx.output_file.write((const char*)imm, sizeof(uint32_t)); break;
        default: break;
    }
}

template<size_t IMM_SIZE, uint8_t DISP_MODE> static void generate_warning_mi(
    Context& ctx,
    const std::vector<uint8_t> prefixes,
    uint8_t op,
    const MemoryOperand& mmop,
    uint64_t imm
) {
    if (IMM_SIZE != 0) {
        print_size_warning<IMM_SIZE>(ctx, imm);
    }
    generate_mi<IMM_SIZE, DISP_MODE>(ctx, prefixes, op, mmop, imm);
}

void x86_format_mi(Context& ctx, const FormatMI& fparams) {
    const auto& imm = fparams.imm;
    const auto& size_override = fparams.size_override;

    MemoryOperand mmop;
    if (make_modrm_sib(ctx, fparams.mdesc, fparams.default_reg_v, mmop)) {
        switch (mmop.size) {
            case 16: {
                if (ctx.b_mode == BitsMode::M16) {
                    if (size_override == 8) {
                        generate_warning_mi<8, 16>(ctx, {}, fparams.r8_imm8_op, mmop, imm);
                    }
                    else if (size_override == 0 || size_override == 16) {
                        if (test_number<int8_t>(imm)) {
                            generate_mi<8, 16>(ctx, {}, fparams.r_def_imm8_op, mmop, imm);
                        }
                        else {
                            generate_warning_mi<16, 16>(ctx, {}, fparams.r_imm_def_op, mmop, imm);
                        }
                    }
                    else if (size_override == 32) {
                        if (test_number<int8_t>(imm)) {
                            generate_mi<8, 16>(ctx, { 0x66 }, fparams.r_def_imm8_op, mmop, imm);
                        }
                        else {
                            generate_warning_mi<32, 16>(ctx, { 0x66 }, fparams.r_imm_def_op, mmop, imm);
                        }
                    }
                    else {
                        std::cerr << std::format(
                            "Error on line {}: 64 bits addressing is unsupported in 16 bits mode",
                            ctx.line_no
                        ) << std::endl;
                        ctx.on_error = true;
                        return;
                    }
                }
                else if (ctx.b_mode == BitsMode::M32) {
                    if (size_override == 8) {
                        generate_warning_mi<8, 16>(ctx, { 0x67 }, fparams.r8_imm8_op, mmop, imm);
                    }
                    else if (size_override == 16) {
                        if (test_number<int8_t>(imm)) {
                            generate_mi<8, 16>(ctx, { 0x66, 0x67 }, fparams.r_def_imm8_op, mmop, imm);
                        }
                        else {
                            generate_warning_mi<16, 16>(ctx, { 0x66, 0x67 }, fparams.r_imm_def_op, mmop, imm);
                        }
                    }
                    else if (size_override == 0 || size_override == 32) {
                        if (test_number<int8_t>(imm)) {
                            generate_mi<8, 16>(ctx, { 0x67 }, fparams.r_def_imm8_op, mmop, imm);
                        }
                        else {
                            generate_mi<32, 16>(ctx, { 0x67 }, fparams.r_imm_def_op, mmop, imm);
                        }
                    }
                    else {
                        std::cerr << std::format(
                            "Error on line {}: 64 bits addressing is unsupported in 32 bits mode",
                            ctx.line_no
                        ) << std::endl;
                        ctx.on_error = true;
                        return;
                    }
                }
                else {
                    std::cerr << std::format(
                        "Error on line {}: 64-bit instructions/operands are currently unsupported",
                        ctx.line_no
                    ) << std::endl;
                    ctx.on_error = true;
                    return;
                }
                break;
            }
            case 32: {
                if (ctx.b_mode == BitsMode::M16) {
                    if (size_override == 8) {
                        generate_warning_mi<8, 32>(ctx, { 0x67 }, fparams.r8_imm8_op, mmop, imm);
                    }
                    else if (size_override == 0 || size_override == 16) {
                        if (test_number<int8_t>(imm)) {
                            generate_mi<8, 32>(ctx, { 0x67 }, fparams.r_def_imm8_op, mmop, imm);
                        }
                        else {
                            generate_warning_mi<16, 32>(ctx, { 0x67 }, fparams.r_imm_def_op, mmop, imm);
                        }
                    }
                    else if (size_override == 32) {
                        if (test_number<int8_t>(imm)) {
                            generate_mi<8, 32>(ctx, { 0x66, 0x67 }, fparams.r_def_imm8_op, mmop, imm);
                        }
                        else {
                            generate_warning_mi<32, 32>(ctx, { 0x66, 0x67 }, fparams.r_imm_def_op, mmop, imm);
                        }
                    }
                    else {
                        std::cerr << std::format(
                            "Error on line {}: 64 bits addressing is unsupported in 16 bits mode",
                            ctx.line_no
                        ) << std::endl;
                        ctx.on_error = true;
                        return;
                    }
                }
                else if (ctx.b_mode == BitsMode::M32) {
                    if (size_override == 8) {
                        generate_warning_mi<8, 32>(ctx, {}, fparams.r8_imm8_op, mmop, imm);
                    }
                    else if (size_override == 16) {
                        if (test_number<int8_t>(imm)) {
                            generate_mi<8, 32>(ctx, { 0x66 }, fparams.r_def_imm8_op, mmop, imm);
                        }
                        else {
                            generate_warning_mi<16, 32>(ctx, { 0x66 }, fparams.r_imm_def_op, mmop, imm);
                        }
                    }
                    else if (size_override == 0 || size_override == 32) {
                        if (test_number<int8_t>(imm)) {
                            generate_mi<8, 32>(ctx, {}, fparams.r_def_imm8_op, mmop, imm);
                        }
                        else {
                            generate_warning_mi<32, 32>(ctx, {}, fparams.r_imm_def_op, mmop, imm);
                        }
                    }
                    else {
                        std::cerr << std::format(
                            "Error on line {}: 64 bits addressing is unsupported in 32 bits mode",
                            ctx.line_no
                        ) << std::endl;
                        ctx.on_error = true;
                        return;
                    }
                }
                else {
                    std::cerr << std::format(
                        "Error on line {}: 64-bit instructions/operands are currently unsupported",
                        ctx.line_no
                    ) << std::endl;
                    ctx.on_error = true;
                    return;
                }
                break;
            }
            default: {
                std::cerr << std::format(
                    "Error on line {}: 64-bit addressing is unsupported",
                    ctx.line_no
                ) << std::endl;
                ctx.on_error = true;
                return;
            }
        }
    }
    else {
        std::cerr << std::format(
            "Error on line {}: Invalid memory descriptor",
            ctx.line_no
        ) << std::endl;
        ctx.on_error = true;
        return;
    }
}

void x86_format_rr(Context& ctx, const std::string_view& instruction, const FormatRR& fparams) {
    if (fparams.reg_source_size != fparams.reg_dest_size) {
        std::cerr << std::format(
            "Error on line {}: Mismatched operand sizes for `{}`",
            ctx.line_no,
            instruction
        ) << std::endl;
        ctx.on_error = true;
        return;
    }

    const uint8_t modrm = build_modrm_core(
        REGISTERS_ENCODING.at(fparams.reg_dest),
        REGISTERS_ENCODING.at(fparams.reg_source),
        0b11
    );

    switch (fparams.reg_source_size) {
        case  8: ctx.output_file.put(fparams.r8_op); ctx.output_file.put(modrm); break;
        case 16: {
            if (ctx.b_mode == BitsMode::M32 || ctx.b_mode == BitsMode::M64) {
                ctx.output_file.put(0x66);
            }
            ctx.output_file.put(fparams.r_def_op);
            ctx.output_file.put(modrm);
            break;
        }
        case 32: {
            if (ctx.b_mode == BitsMode::M16) {
                ctx.output_file.put(0x66);
            }
            ctx.output_file.put(fparams.r_def_op);
            ctx.output_file.put(modrm);
            break;
        }
        default: {
            std::cerr << std::format(
                "Error on line {}: Unsupported format/size for `{}`",
                ctx.line_no,
                instruction
            ) << std::endl;
            ctx.on_error = true;
            return;
        }
    }
}

template<uint8_t DISP_MODE> static void generate_mr(
    Context& ctx,
    const std::vector<uint8_t>& prefixes,
    const std::vector<uint8_t>& other_prefixes,
    const std::vector<uint8_t>& ex_prefixes,
    uint8_t op,
    const MemoryOperand& mmop
) {
    if (!ex_prefixes.empty()) {
        for (const auto& p : prefixes) {
            if (!ex_prefixes.contains(p)) {
                ctx.output_file.put(p);
            }
        }
    }
    else {
        ctx.output_file.write((const char*)prefixes.data(), prefixes.size());
    }

    if (!other_prefixes.empty()) {
        ctx.output_file.write((const char*)ohter_prefixes.data(), other_prefixes.size());
    }
    else {
        ctx.output_file.put(op);
    }

    ctx.output_file.put(mmop.modrm);

    if (mmop.has_sib) {
        ctx.output_file.put(mmop.sib);
    }

    if (DISP_MODE == 16) {
        output_disp_16(ctx, mmop.disp_size, mmop.disp);
    }
    else {
        output_disp_32(ctx, mmop.disp_size, mmop.disp);
    }
}

void x86_format_mr(Context& ctx, const FormatMR& fparams) {
    MemoryOperand mmop;

    if (make_modrm_sib(ctx, fparams.mdesc, fparams.default_reg_v, mmop)) {
        if (fparams.size_override != 0 && (int32_t)(fparams.size_override) != fparams.reg_size) {
            std::cerr << std::format(
                "Error on line {}: Mismatched operand sizes",
                ctx.line_no
            );
            ctx.on_error = true;
            return;
        }

        switch (mmop.size) {
            case 16: {
                switch (ctx.b_mode) {
                    case BitsMode::M16: {
                        switch (fparams.reg_size) {
                            case  8: generate_mr<16>(ctx, {}, fparams.prefixes, fparams.ex_prefixes, fparams.r8_rm8_op, mmop); break;
                            case 16: generate_mr<16>(ctx, {}, fparams.prefixes, fparams.ex_prefixes, fparams.r_rm_def_op, mmop); break;
                            case 32: generate_mr<16>(ctx, { 0x66 }, fparams.prefixes, fparams.ex_prefixes, fparams.r_rm_def_op, mmop); break;
                            default: {
                                std::cerr << std::format(
                                    "Error on line {}: 64-bit registers use is unsupported in 16 bits mode",
                                    ctx.line_no
                                );
                                ctx.on_error = true;
                                return;
                            }
                        }
                        break;
                    }
                    case BitsMode::M32: {
                        switch (fparams.reg_size) {
                            case  8: generate_mr<16>(ctx, { 0x67 }, fparams.prefixes, fparams.ex_prefixes, fparams.r8_rm8_op, mmop); break;
                            case 16: generate_mr<16>(ctx, { 0x66, 0x67}, fparams.prefixes, fparams.ex_prefixes, fparams.r_rm_def_op, mmop); break;
                            case 32: generate_mr<16>(ctx, { 0x67 }, fparams.prefixes, fparams.ex_prefixes, fparams.r_rm_def_op, mmop); break;
                            default: {
                                std::cerr << std::format(
                                    "Error on line {}: 64-bit registers use is unsupported in 32 bits mode",
                                    ctx.line_no
                                ) << std::endl;
                                ctx.on_error = true;
                                return;
                            }
                        }
                        break;
                    }
                    default: {
                        std::cerr << std::format(
                            "Error on line {}: 64-bit instructions/operands are currently unsupported",
                            ctx.line_no
                        ) << std::endl;
                        ctx.on_error = true;
                        return;
                    }
                }
                break;
            }
            case 32: {
                switch (ctx.b_mode) {
                    case BitsMode::M16: {
                        switch (fparams.reg_size) {
                            case  8: generate_mr<32>(ctx, { 0x67 }, fparams.prefixes, fparams.ex_prefixes, fparams.r8_rm8_op, mmop); break;
                            case 16: generate_mr<32>(ctx, { 0x67 }, fparams.prefixes, fparams.ex_prefixes, fparams.r_rm_def_op, mmop); break;
                            case 32: generate_mr<32>(ctx, { 0x66, 0x67 }, fparams.prefixes, fparams.ex_prefixes, fparams.r_rm_def_op, mmop); break;
                            default: {
                                std::cerr << std::format(
                                    "Error on line {}: 64-bit registers use is unsupported in 16 bits mode",
                                    ctx.line_no
                                ) << std::endl;
                                ctx.on_error = true;
                                return;
                            }
                        }
                        break;
                    }
                    case BitsMode::M32: {
                        switch (fparams.reg_size) {
                            case  8: generate_mr<32>(ctx, {}, fparams.prefixes, fparams.ex_prefixes, fparams.r8_rm8_op, mmop); break;
                            case 16: generate_mr<32>(ctx, { 0x66 }, fparams.prefixes, fparams.ex_prefixes, fparams.r_rm_def_op, mmop); break;
                            case 32: generate_mr<32>(ctx, {}, fparams.prefixes, fparams.ex_prefixes, fparams.r_rm_def_op, mmop); break;
                            default: {
                                std::cerr << std::format(
                                    "Error on line {}: 64-bit registers use is unsupported in 32 bits mode",
                                    ctx.line_no
                                ) << std::endl;
                                ctx.on_error = true;
                                return;
                            }
                        }
                        break;
                    }
                    default: {
                        std::cerr << std::format(
                            "Error on line {}: 64-bit instructions/operands are currently unsupported",
                            ctx.line_no
                        ) << std::endl;
                        ctx.on_error = true;
                        return;
                    }
                }
                break;
            }
            default: {
                std::cerr << std::format(
                    "Error on line {}: 64-bit addressing is unsupported",
                    ctx.line_no
                ) << std::endl;
                ctx.on_error = true;
                return;
            }
        }
    }
    else {
        std::cerr << std::format(
            "Error on line {}: Invalid memory operand",
            ctx.line_no
        ) << std::endl;
        ctx.on_error = true;
        return;
    }
}
