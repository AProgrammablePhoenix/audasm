#include <algorithm>
#include <iostream>
#include <string_view>
#include <vector>

#include "context.hpp"
#include "memory.hpp"
#include "parsing_utils.hpp"
#include "registers.hpp"

namespace {
    static inline bool invalid_16_bit_repetition(
        Context& ctx,
        const std::string_view& atom,
        const std::string_view& s
    ) {
        std::cerr << std::format(
            "Error on line {}: Illegal repetition of register `{}` in 16-bit memory operand `[{}]`",
            ctx.line_no,
            atom,
            s
        ) << std::endl;
        ctx.on_error = true;
        return false;
    }

    static inline bool invalid_16_bit_combination(
        Context& ctx,
        const std::string_view& s
    ) {
        std::cerr << std::format(
            "Error on line {}: Illegal combination of registers in 16-bit memory operand `[{}]`",
            ctx.line_no,
            s
        ) << std::endl;
        ctx.on_error = true;
        return false;
    }

    static inline void parse_quark(
        Context& ctx,
        const std::vector<std::string_view>& quarks,
        const std::string& atom,
        const std::string& rs,
        size_t x,
        size_t y,
        uint8_t& register_encoding,
        uint8_t& scale
    ) {
        const auto& [reg, rsize] = REGISTERS.at(quarks[x].data());
        if (rsize != 32) {
            std::cerr << std::format(
                "Error on line {}: Invalid width for register `{}` in scaled index `{}` in memory operand `[{}]`",
                ctx.line_no,
                quarks[x],
                atom,
                rs
            ) << std::endl;
            ctx.on_error = true;
            return;
        }

        register_encoding = REGISTERS_ENCODING.at(reg);
        uint64_t n;

        if (
            !parse_number(ctx, quarks[y], n)
            || !(n == 1 || n == 2 || n == 4 || n == 8)
        ) {
            std::cerr << std::format(
                "Error on line {}: invalid scale `{}` in memory operand `[{}]`, must be 1, 2, 4 or 8 ; default is 1 if absent",
                ctx.line_no,
                quarks[y],
                rs
            ) << std::endl;
            ctx.on_error = true;
            return;
        }

        scale = (uint8_t)n;
    }
}

#define match_16_bit_reg(X, Y)                                  \
    do {                                                        \
        if (desc.X) {                                           \
            return invalid_16_bit_repetition(ctx, atom, rs);    \
        }                                                       \
        else if (desc.Y) {                                      \
            return invalid_16_bit_combination(ctx, rs);         \
        }                                                       \
        desc.X = true;                                          \
    } while (0)

bool parse_memory(
    Context& ctx,
    std::string rs,
    MemoryOperandDescriptor& mdesc
) {
    std::string s = rs;
    s.erase(std::remove(s.begin(), s.end(), ' '), s.end());

    std::string atom = "";
    bool is_adding = true;

    MemoryOperandDescriptor desc = {
        .size   = 0,
        .bx     = false,
        .bp     = false,
        .si     = false,
        .di     = false,
        .disp   = 0,
        .index  = 0xFF,
        .scale  = 0,
        .base   = 0xFF
    };

    for (const char& c : s) {
        if (c != '+' && c != '-') {
            atom.push_back(c);
        }
        else {
            if (REGISTERS.contains(atom)) {
                const auto& [reg, rsize] = REGISTERS.at(atom);
                switch (rsize) {
                    case 8: {
                        std::cerr << std::format(
                            "Error on line {}: Invalid memory operand `[{}]` (illegal use of 8-bit register `{}`)",
                            ctx.line_no,
                            rs,
                            atom
                        ) << std::endl;
                        ctx.on_error = true;
                        return false;
                    }
                    case 16: {
                        if (desc.size == 0) {
                            desc.size = 16;
                        }
                        else if (desc.size != 16) {
                            std::cerr << std::format(
                                "Error on line {}: Invalid combination of 16-bit register `{}` in {}-bit memory operand `[{}]`",
                                ctx.line_no,
                                atom,
                                desc.size,
                                rs
                            ) << std::endl;
                            ctx.on_error = true;
                            return false;
                        }

                        switch (reg) {
                            case AsmRegister::BX: {
                                match_16_bit_reg(bx, bp);
                                break;
                            }
                            case AsmRegister::BP: {
                                match_16_bit_reg(bp, bx);
                                break;
                            }
                            case AsmRegister::SI: {
                                match_16_bit_reg(si, di);
                                break;
                            }
                            case AsmRegister::DI: {
                                match_16_bit_reg(di, si);
                                break;
                            }
                            default: {
                                std::cerr << std::format(
                                    "Error on line {}: Use of invalid 16-bit register `{}` in 16-bit memory operand `[{}]`",
                                    ctx.line_no,
                                    atom,
                                    rs
                                ) << std::endl;
                                ctx.on_error = true;
                                return false;
                            }
                        }

                        break;
                    }
                    case 32: {
                        if (desc.size == 0) {
                            desc.size = 32;
                        }
                        else if (desc.size != 32) {
                            std::cerr << std::format(
                                "Error on line {}: Invalid combination of 32-bit register `{}` in {}-bit memory operand `[{}]`",
                                ctx.line_no,
                                atom,
                                desc.size,
                                rs
                            ) << std::endl;
                            ctx.on_error = true;
                            return false;
                        }

                        uint8_t encoding = REGISTERS_ENCODING.at(reg);

                        if (desc.base == 0xFF) {
                            desc.base = encoding;
                        }
                        else if (desc.index == 0xFF) {
                            desc.scale = 1;
                            desc.index = encoding;
                        }
                        else {
                            if (encoding == desc.base) {
                                if (desc.index == 0xFF || desc.scale == 1) {
                                    desc.base = desc.index;
                                    desc.index = encoding;
                                    desc.scale = 2;
                                }
                                else {
                                    std::cerr << std::format(
                                        "Error on line {}: Invalid repetition of 32-bit register `{}` in memory operand `[{}]`, consider using the format `[SCALE * INDEX + BASE + DISP]`",
                                        ctx.line_no,
                                        atom,
                                        rs
                                    ) << std::endl;
                                    ctx.on_error = true;
                                    return false;
                                }
                            }
                            else if (encoding == desc.index) {
                                desc.scale += is_adding ? 1 : -1;
                            }
                            else {
                                std::cerr << std::format(
                                    "Error on line {}: Invalid use of third 32-bit register `{}` in memory operand `[{}]`, consider using the format `[SCALE * INDEX + BASE + DISP]`",
                                    ctx.line_no,
                                    atom,
                                    rs
                                ) << std::endl;
                                ctx.on_error = true;
                                return false;
                            }
                        }

                        break;
                    }
                    default: {
                        std::cerr << std::format(
                            "Error on line {}: Unsupported width for {}-bit register `{}` in memory operand `[{}]`",
                            ctx.line_no,
                            rsize == -1 ? 16 : rsize,
                            atom,
                            rs
                        ) << std::endl;
                        ctx.on_error = true;
                        return false;
                    }
                }
            }
            else if (atom.contains('*')) {
                std::vector<std::string_view> quarks = split_string(atom, '*');

                uint8_t register_encoding;
                uint8_t scale;

                if (quarks.size() != 2) {
                    std::cerr << std::format(
                        "Error on line {}: Too many fields in scaled index `{}` in memory operand `[{}]`, consider using the format `[SCALe * INDEX + BASE + DISP]`",
                        ctx.line_no,
                        atom,
                        rs
                    ) << std::endl;
                    ctx.on_error = true;
                    return false;
                }
                else if (REGISTERS.contains(quarks[0].data())) {
                    parse_quark(ctx, quarks, atom, rs, 0, 1, register_encoding, scale);
                    if (ctx.on_error) {
                        return false;
                    }
                }
                else if (REGISTERS.contains(quarks[1].data())) {
                    parse_quark(ctx, quarks, atom, rs, 1, 0, register_encoding, scale);
                    if (ctx.on_error) {
                        return false;
                    }
                }
                else {
                    std::cerr << std::format(
                        "Error on line {}: Invalid scaled index `{}` in memory operand `[{}]`, no memory operand is a valid register",
                        ctx.line_no,
                        atom,
                        rs
                    ) << std::endl;
                    ctx.on_error = true;
                    return false;
                }

                if (!is_adding) {
                    scale = -scale;
                }

                if (desc.index == 0xFF) {
                    if (desc.index == register_encoding) {
                        desc.scale += scale;
                    }
                    else if (desc.base == 0xFF && desc.scale == 1) {
                        desc.base = desc.index;
                        desc.scale = scale;
                    }
                    else {
                        std::cerr << std::format(
                            "Error on line {}: Cannot have two scaled indexes in memory operand `[{}]`, consider using the format `[SCALE * INDEX + BASE + DISP]`",
                            ctx.line_no,
                            rs
                        ) << std::endl;
                        ctx.on_error = true;
                        return false;
                    }
                }
                else if (desc.base == register_encoding) {
                    desc.base = 0xFF;
                    desc.scale = scale;
                }
                else {
                    desc.scale = scale;
                }

                desc.index = register_encoding;
            }
            else {
                uint64_t n;
                if (!parse_number(ctx, atom, n)) {
                    std::cerr << std::format(
                        "Error on line {}: Invalid expression `{}` in memory operand `[{}]`, consider using the format `[SCALE * INDEX + BASE + DISP]`",
                        ctx.line_no,
                        atom,
                        rs
                    ) << std::endl;
                    ctx.on_error = true;
                    return false;
                }

                int64_t sn = (int64_t)n;
                if (!test_number<int32_t>(sn)) {
                    sn = (int64_t)((int32_t)sn);
                    std::cerr << std::format(
                        "Warning on line {}: Displacement magnitude of `{}` is too large, applying modulo 2^32, might cause unwanted or undefined behavior",
                        ctx.line_no,
                        atom
                    ) << std::endl;
                }
                desc.disp += is_adding ? sn : -sn;
            }

            is_adding = c == '+';
            atom.clear();
        }
    }

    switch (desc.scale) {
        case 0: case 1: case 2: case 4: case 8:
            mdesc = desc;
            return true;
        default: {
            std::cerr << std::format(
                "Error on line {}: Invalid scale `{}` in memory operand `[{}]`, valid values are 1, 2, 4 and 8",
                ctx.line_no,
                desc.scale,
                rs
            ) << std::endl;
            ctx.on_error = true;
            return false;
        }
    }
}

uint8_t build_modrm_core(uint8_t rm, uint8_t reg, uint8_t mod) {
    return ((mod & 0x3) << 6) | ((reg & 0x7) << 3) | (rm & 0x7);
}

bool build_16bit_modrm(
    Context& ctx,
    const MemoryOperandDescriptor& desc,
    uint8_t rm,
    uint8_t reg,
    MemoryOperand& mop
) {
    if (desc.disp == 0) {
        mop = {
            .size       = 16,
            .modrm      = build_modrm_core(rm, reg, 0b00),
            .has_sib    = false,
            .sib        = 0,
            .disp_size  = 0,
            .disp       = 0
        };
        return true;
    }
    else if (test_number<int8_t>(desc.disp)) {
        mop = {
            .size       = 16,
            .modrm      = build_modrm_core(rm, reg, 0b01),
            .has_sib    = false,
            .sib        = 0,
            .disp_size  = 8,
            .disp       = (uint64_t)desc.disp
        };
        return true;
    }
    else if (test_number<int16_t>(desc.disp)) {
        mop = {
            .size       = 16,
            .modrm      = build_modrm_core(rm, reg, 0b10),
            .has_sib    = false,
            .sib        = 0,
            .disp_size  = 16,
            .disp       = (uint64_t)desc.disp
        };
        return true;
    }
    else {
        std::cerr << std::format(
            "Error on line {}: Displacement `{}` is too large for 16-bit addressing mode",
            ctx.line_no,
            desc.disp
        ) << std::endl;
        ctx.on_error = true;
        return false;
    }
}

uint8_t build_sib_core(uint8_t base, uint8_t index, uint8_t scale) {
    uint8_t scl;
    
    switch (scale) {
        case 2: {
            scl = 0b01;
            break;
        }
        case 4: {
            scl = 0b10;
            break;
        }
        case 8: {
            scl = 0b11;
            break;
        }
        default: scl = 0b00;
    }

    return (scl << 6) | ((index & 0x7) << 3) | (base & 0x7);
}

bool make_modrm_sib(
    Context& ctx,
    MemoryOperandDescriptor desc,
    uint8_t reg_v,
    MemoryOperand& mop
) {
    switch (desc.size) {
        case 16: {
            if (desc.bx) {
                if (desc.si) {
                    return build_16bit_modrm(ctx, desc, 0b000, reg_v, mop);
                }
                else if (desc.di) {
                    return build_16bit_modrm(ctx, desc, 0b001, reg_v, mop);
                }
                else {
                    return build_16bit_modrm(ctx, desc, 0b111, reg_v, mop);
                }
            }
            else if (desc.bp) {
                if (desc.si) {
                    return build_16bit_modrm(ctx, desc, 0b010, reg_v, mop);
                }
                else if (desc.di) {
                    return build_16bit_modrm(ctx, desc, 0b011, reg_v, mop);
                }
                else if (test_number<int8_t>(desc.disp)) {
                    mop = {
                        .size       = 16,
                        .modrm      = build_modrm_core(0b110, reg_v, 0b01),
                        .has_sib    = false,
                        .sib        = 0,
                        .disp_size  = 8,
                        .disp       = (uint64_t)desc.disp
                    };
                    return true;
                }
                else {
                    mop = {
                        .size       = 16,
                        .modrm      = build_modrm_core(0b110, reg_v, 0b10),
                        .has_sib    = false,
                        .sib        = 0,
                        .disp_size  = 16,
                        .disp       = (uint64_t)desc.disp
                    };
                    return true;
                }
            }
            else if (desc.si) {
                return build_16bit_modrm(ctx, desc, 0b100, reg_v, mop);
            }
            else if (desc.di) {
                return build_16bit_modrm(ctx, desc, 0b101, reg_v, mop);
            }
            else {
                mop = {
                    .size       = 16,
                    .modrm      = build_modrm_core(0b110, reg_v, 0b00),
                    .has_sib    = false,
                    .sib        = 0,
                    .disp_size  = 16,
                    .disp       = (uint64_t)desc.disp
                };
                return true;
            }

            break;
        }
        case 32: {
            const uint8_t esp_encoding = REGISTERS_ENCODING.at(AsmRegister::ESP);
            const uint8_t ebp_encoding = REGISTERS_ENCODING.at(AsmRegister::EBP);

            if (desc.index == esp_encoding) {
                if (desc.scale != 1) {
                    std::cerr << std::format(
                        "Error on line {}: Cannot use ESP with a memory index",
                        ctx.line_no
                    ) << std::endl;
                    ctx.on_error = true;
                    return false;
                }
                std::swap(desc.base, desc.index);
            }

            if (desc.index == 0xFF && desc.base != esp_encoding) {
                if (desc.disp == 0) {
                    if (desc.base != ebp_encoding) {
                        mop = {
                            .size       = 32,
                            .modrm      = build_modrm_core(desc.base, reg_v, 0b00),
                            .has_sib    = false,
                            .sib        = 0,
                            .disp_size  = 0,
                            .disp       = 0
                        };
                        return true;
                    }
                    else if (test_number<int8_t>(desc.disp)) {
                        mop = {
                            .size       = 32,
                            .modrm      = build_modrm_core(desc.base, reg_v, 0b01),
                            .has_sib    = false,
                            .sib        = 0,
                            .disp_size  = 8,
                            .disp       = (uint64_t)desc.disp
                        };
                        return true;
                    }
                    else {
                        mop = {
                            .size       = 32,
                            .modrm      = build_modrm_core(desc.base, reg_v, 0b10),
                            .has_sib    = false,
                            .sib        = 0,
                            .disp_size  = 32,
                            .disp       = (uint64_t)desc.disp
                        };
                        return true;
                    }
                }
            }

            const uint8_t sib_rm = esp_encoding;
            const uint8_t disp_size = test_number<int8_t>(desc.disp) ? 8 : 32;
            const uint8_t mmod = (disp_size == 8) ? 0b01 : 0b10;

            if (desc.base == 0xFF && desc.index == 0xFF) {
                mop = {
                    .size       = 32,
                    .modrm      = build_modrm_core(sib_rm, reg_v, 0b00),
                    .has_sib    = true,
                    .sib        = build_sib_core(0b101, 0b100, 0b00),
                    .disp_size  = 32,
                    .disp       = (uint64_t)desc.disp
                };
                return true;
            }
            else if (desc.base == ebp_encoding) {
                if (desc.index == 0xFF) {
                    mop = {
                        .size       = 32,
                        .modrm      = build_modrm_core(sib_rm, reg_v, mmod),
                        .has_sib    = true,
                        .sib        = build_sib_core(ebp_encoding, esp_encoding, 0b00),
                        .disp_size  = disp_size,
                        .disp       = (uint64_t)desc.disp
                    };
                    return true;
                }
                else {
                    mop = {
                        .size       = 32,
                        .modrm      = build_modrm_core(sib_rm, reg_v, mmod),
                        .has_sib    = true,
                        .sib        = build_sib_core(ebp_encoding, desc.index, desc.scale),
                        .disp_size  = disp_size,
                        .disp       = (uint64_t)desc.disp
                    };
                    return true;
                }
            }
            else {
                if (desc.index == 0xFF) {
                    if (desc.disp == 0) {
                        mop = {
                            .size       = 32,
                            .modrm      = build_modrm_core(sib_rm, reg_v, 0b00),
                            .has_sib    = true,
                            .sib        = build_sib_core(desc.base, esp_encoding, 0b00),
                            .disp_size  = 0,
                            .disp       = 0
                        };
                        return true;
                    }
                    else {
                        mop = {
                            .size       = 32,
                            .modrm      = build_modrm_core(sib_rm, reg_v, mmod),
                            .has_sib    = true,
                            .sib        = build_sib_core(desc.base, esp_encoding, 0b00),
                            .disp_size  = disp_size,
                            .disp       = (uint64_t)desc.disp
                        };
                        return true;
                    }
                }
                else if (desc.base == 0xFF) {
                    mop = {
                        .size       = 32,
                        .modrm      = build_modrm_core(sib_rm, reg_v, 0b00),
                        .has_sib    = true,
                        .sib        = build_sib_core(ebp_encoding, desc.index, desc.scale),
                        .disp_size  = 32,
                        .disp       = (uint64_t)desc.disp
                    };
                    return true;
                }
                else if (desc.disp == 0) {
                    mop = {
                        .size       = 32,
                        .modrm      = build_modrm_core(sib_rm, reg_v, 0b00),
                        .has_sib    = true,
                        .sib        = build_sib_core(desc.base, desc.index, desc.scale),
                        .disp_size  = 0,
                        .disp       = 0
                    };
                    return true;
                }
                else {
                    mop = {
                        .size       = 32,
                        .modrm      = build_modrm_core(sib_rm, reg_v, mmod),
                        .has_sib    = true,
                        .sib        = build_sib_core(desc.base, desc.index, desc.scale),
                        .disp_size  = disp_size,
                        .disp       = (uint64_t)desc.disp
                    };
                    return true;
                }
            }

            break;
        }
        default: return false;
    }
}

void output_disp_16(Context& ctx, uint8_t disp_size, uint64_t disp) {
    switch (disp_size) {
        case  8: ctx.output_file.put((uint8_t)disp); break;
        case 16: ctx.output_file.write((const char*)disp, sizeof(uint16_t)); break;
        default: break;
    }
}

void output_disp_32(Context& ctx, uint8_t disp_size, uint64_t disp) {
    switch (disp_size) {
        case  8: ctx.output_file.put((uint8_t)disp); break;
        case 32: ctx.output_file.write((const char*)disp, sizeof(uint32_t)); break;
        default: break;
    }
}
