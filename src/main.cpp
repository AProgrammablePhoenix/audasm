#include <cstdint>
#include <cstddef>

#include <algorithm>
#include <charconv>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "context.hpp"

enum class AsmRegister {
    AL, AH, AX, EAX,
    BL, BH, BX, EBX,
    CL, CH, CX, ECX,
    DL, DH, DX, EDX,
            SI, ESI,
            DI, EDI,
            SP, ESP,
            BP, EBP,
    CS,
    DS,
    ES,
    FS,
    GS,
    SS
};

static std::unordered_map<std::string, std::pair<AsmRegister, int32_t>> REGISTERS = {
    { "AL",     { AsmRegister::AL,   8 } },
    { "AH",     { AsmRegister::AH,   8 } },
    { "AX",     { AsmRegister::AX,  16 } },
    { "EAX",    { AsmRegister::EAX, 32 } },
    { "BL",     { AsmRegister::BL,   8 } },
    { "BH",     { AsmRegister::BH,   8 } },
    { "BX",     { AsmRegister::BX,  16 } },
    { "EBX",    { AsmRegister::EBX, 32 } },
    { "CL",     { AsmRegister::CL,   8 } },
    { "CH",     { AsmRegister::CH,   8 } },
    { "CX",     { AsmRegister::CX,  16 } },
    { "ECX",    { AsmRegister::ECX, 32 } },
    { "DL",     { AsmRegister::DL,   8 } },
    { "DH",     { AsmRegister::DH,   8 } },
    { "DX",     { AsmRegister::DX,  16 } },
    { "EDX",    { AsmRegister::EDX, 32 } },
    { "SI",     { AsmRegister::SI,  16 } },
    { "ESI",    { AsmRegister::ESI, 32 } },
    { "DI",     { AsmRegister::DI,  16 } },
    { "EDI",    { AsmRegister::EDI, 32 } },
    { "SP",     { AsmRegister::SP,  16 } },
    { "ESP",    { AsmRegister::ESP, 32 } },
    { "BP",     { AsmRegister::BP,  16 } },
    { "EBP",    { AsmRegister::EBP, 32 } },
    { "CS",     { AsmRegister::CS,  -1 } },
    { "DS",     { AsmRegister::DS,  -1 } },
    { "ES",     { AsmRegister::ES,  -1 } },
    { "FS",     { AsmRegister::FS,  -1 } },
    { "GS",     { AsmRegister::GS,  -1 } },
    { "SS",     { AsmRegister::SS,  -1 } }
};

static std::unordered_map<AsmRegister, uint8_t> REGISTERS_ENCODING = {
    { AsmRegister::AL, 0b0000 }, { AsmRegister::AX, 0b0000 }, { AsmRegister::EAX, 0b0000 },
    { AsmRegister::CL, 0b0001 }, { AsmRegister::CX, 0b0001 }, { AsmRegister::ECX, 0b0001 },
    { AsmRegister::DL, 0b0010 }, { AsmRegister::DX, 0b0010 }, { AsmRegister::EDX, 0b0010 },
    { AsmRegister::BL, 0b0011 }, { AsmRegister::BX, 0b0011 }, { AsmRegister::EBX, 0b0011 },
    { AsmRegister::AH, 0b0100 }, { AsmRegister::SP, 0b0100 }, { AsmRegister::ESP, 0b0100 },
    { AsmRegister::CH, 0b0101 }, { AsmRegister::BP, 0b0101 }, { AsmRegister::EBP, 0b0101 },
    { AsmRegister::DH, 0b0110 }, { AsmRegister::SI, 0b0110 }, { AsmRegister::ESI, 0b0110 },
    { AsmRegister::BH, 0b0111 }, { AsmRegister::DI, 0b0111 }, { AsmRegister::EDI, 0b0111 },    
    { AsmRegister::ES, 0b0000 },
    { AsmRegister::CS, 0b0001 },
    { AsmRegister::SS, 0b0010 },
    { AsmRegister::DS, 0b0011 },
    { AsmRegister::FS, 0b0100 },
    { AsmRegister::GS, 0b0101 }
};

enum class AsmArgType {
    IMMEDIATE,
    REGISTER,
    MEMORY
};

struct AsmArg {
    AsmArgType type;
    union {
        uint64_t imm;
        std::pair<AsmRegister, int32_t> reg;
    };
};

#define CVEC(...) __VA_ARGS__

#define ZO_I_OPC(opc) \
    ZOInstruction { \
        .opcode = opc, \
        .forbidden_prefixes = {}, \
        .mode_prefix = { .mode = BitsMode::INVALID }, \
        .other_prefixes = {} \
    }

#define ZO_I_BASE(opc, fpf, opf) \
    ZOInstruction { \
        .opcode = opc, \
        .forbidden_prefixes = fpf, \
        .mode_prefix = { .mode = BitsMode::INVALID }, \
        .other_prefixes = opf \
    }

#define ZO_I_EXT(opc, fpf, mpfm, mpf, opf) \
    ZOInstruction { \
        .opcode = opc, \
        .forbidden_prefixes = fpf, \
        .mode_prefix = { .mode = mpfm, .prefix = mpf }, \
        .other_prefixes = opf \
    }

#define ZOIMM_OPC(opc) \
    ZOInstruction { \
        .opcode = opc, \
        .forbidden_prefixes = {}, \
        .mode_prefix = { .mode = BitsMode::INVALID }, \
        .other_prefixes = {}, \
        .hasOptionalImm8 = true \
    }

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

static std::unordered_map<std::string, ZOInstruction> ZOTable = {
    { "AAA",            ZO_I_OPC(0x37) },
    { "AAD",            ZOIMM_OPC(0xD5) },
    { "AAM",            ZOIMM_OPC(0xD4) },
    { "AAS",            ZO_I_OPC(0x3F) },
    { "CBW",            ZO_I_EXT(0x98, {}, BitsMode::M32, 0x66, {}) },
    { "CWDE",           ZO_I_EXT(0x98, {}, BitsMode::M16, 0x66, {}) },
    { "CWD",            ZO_I_EXT(0x99, {}, BitsMode::M32, 0x66, {}) },
    { "CDQ",            ZO_I_EXT(0x99, {}, BitsMode::M16, 0x66, {}) },
    { "CLAC",           ZO_I_BASE(0xCA, CVEC({ 0x66, 0xF2, 0xF3 }), CVEC({ 0x0F, 0x01 })) },
    { "CLC",            ZO_I_OPC(0xF8) },
    { "CLD",            ZO_I_OPC(0xFC) },
    { "CLI",            ZO_I_OPC(0xFA) },
    { "CLTS",           ZO_I_BASE(0x06, {}, CVEC({ 0x0F })) },
    { "CMC",            ZO_I_OPC(0xF5) },
    { "CMPSB",          ZO_I_OPC(0xA6) },
    { "CMPSW",          ZO_I_EXT(0xA7, {}, BitsMode::M32, 0x66, {}) },
    { "CMPSD",          ZO_I_EXT(0xA7, {}, BitsMode::M16, 0x66, {}) },
    { "CPUID",          ZO_I_BASE(0xA2, {}, CVEC({0x0F})) },
    { "DAA",            ZO_I_OPC(0x27) },
    { "DAS",            ZO_I_OPC(0x2F) },
    { "ENDBR32",        ZO_I_BASE(0xFB, {}, CVEC({ 0xF3, 0x0F, 0x1E })) },
    { "ENDBR64",        ZO_I_BASE(0xFA, {}, CVEC({ 0xF3, 0x0F, 0x1E })) },
    { "HLT",            ZO_I_OPC(0xF4) },
    { "INSB",           ZO_I_OPC(0x6C) },
    { "INSW",           ZO_I_EXT(0x6D, {}, BitsMode::M32, 0x66, {}) },
    { "INSD",           ZO_I_EXT(0x6D, {}, BitsMode::M16, 0x66, {}) },
    { "INT1",           ZO_I_OPC(0xF1) },
    { "INT3",           ZO_I_OPC(0xCC) },
    { "INTO",           ZO_I_OPC(0xCE) },
    { "INVD",           ZO_I_BASE(0x08, {}, CVEC({ 0x0F })) },
    { "IRET",           ZO_I_OPC(0xCF) },
    { "IRETD",          ZO_I_EXT(0xCF, {}, BitsMode::M16, 0x66, {}) },
    { "LAHF",           ZO_I_OPC(0x9F) },
    { "LEAVE",          ZO_I_OPC(0xC9) },
    { "LFENCE",         ZO_I_BASE(0xE8, CVEC({ 0x66, 0xF2, 0xF3 }), CVEC({ 0x0F, 0xAE })) },
    { "LODSB",          ZO_I_OPC(0xAC) },
    { "LODSW",          ZO_I_EXT(0xAD, {}, BitsMode::M32, 0x66, {}) },
    { "LODSD",          ZO_I_EXT(0xAD, {}, BitsMode::M16, 0x66, {}) },
    { "MFENCE",         ZO_I_BASE(0xF0, CVEC({ 0x66, 0xF2, 0xF3 }), CVEC({ 0x0F, 0xAE })) },
    { "MONITOR",        ZO_I_BASE(0xC8, {}, CVEC({ 0x0F, 0x01 })) },
    { "MOVSB",          ZO_I_OPC(0xA4) },
    { "MOVSW",          ZO_I_EXT(0xA5, {}, BitsMode::M32, 0x66, {}) },
    { "MOVSD",          ZO_I_EXT(0xA5, {}, BitsMode::M16, 0x66, {}) },
    { "MWAIT",          ZO_I_BASE(0xC9, {}, CVEC({ 0x0F, 0x01 })) },
    { "OUTSB",          ZO_I_OPC(0x6E) },
    { "OUTSW",          ZO_I_EXT(0x6F, {}, BitsMode::M32, 0x66, {}) },
    { "OUTSD",          ZO_I_EXT(0x6F, {}, BitsMode::M16, 0x66, {}) },
    { "PAUSE",          ZO_I_BASE(0x90, {}, CVEC({ 0xF3 })) },
    { "PCONFIG",        ZO_I_BASE(0xC5, CVEC({ 0x66, 0xF2, 0xF3 }), CVEC({ 0x0F, 0x01 })) },
    { "POPA",           ZO_I_EXT(0x61, {}, BitsMode::M32, 0x66, {}) },
    { "POPAD",          ZO_I_EXT(0x61, {}, BitsMode::M16, 0x66, {}) },
    { "POPF",           ZO_I_EXT(0x9D, {}, BitsMode::M32, 0x66, {}) },
    { "POPFD",          ZO_I_EXT(0x9D, {}, BitsMode::M16, 0x66, {}) },
    { "PUSHA",          ZO_I_EXT(0x60, {}, BitsMode::M32, 0x66, {}) },
    { "PUSHAD",         ZO_I_EXT(0x60, {}, BitsMode::M16, 0x66, {}) },
    { "PUSHF",          ZO_I_EXT(0x9C, {}, BitsMode::M32, 0x66, {}) },
    { "PUSHFD",         ZO_I_EXT(0x9C, {}, BitsMode::M16, 0x66, {}) },
    { "RDMSR",          ZO_I_BASE(0x32, {}, CVEC({ 0x0F })) },
    { "RDPKRU",         ZO_I_BASE(0xEE, CVEC({ 0x66, 0xF2, 0xF3 }), CVEC({ 0x0F, 0x01 })) },
    { "RDPMC",          ZO_I_BASE(0x33, {}, CVEC({ 0x0F })) },
    { "RDTSC",          ZO_I_BASE(0x31, {}, CVEC({ 0x0F })) },
    { "RDTSCP",         ZO_I_BASE(0xF9, {}, CVEC({ 0x0F, 0x01 })) },
    { "RSM",            ZO_I_BASE(0xAA, {}, CVEC({ 0x0F })) },
    { "SAHF",           ZO_I_OPC(0x9E) },
    { "SAVEPREVSSP",    ZO_I_BASE(0xEA, {}, CVEC({ 0xF3, 0x0F, 0x01 })) },
    { "SCASB",          ZO_I_OPC(0xAE) },
    { "SCASW",          ZO_I_EXT(0xAF, {}, BitsMode::M32, 0x66, {}) },
    { "SCASD",          ZO_I_EXT(0xAF, {}, BitsMode::M16, 0x66, {}) },
    { "SERIALIZE",      ZO_I_BASE(0xE8, CVEC({ 0x66, 0xF2, 0xF3 }), CVEC({ 0x0F, 0x01 })) },
    { "SETSSBSY",       ZO_I_BASE(0xE8, {}, CVEC({ 0xF3, 0x0F, 0x01 })) },
    { "SFENCE",         ZO_I_BASE(0xF8, CVEC({ 0x66, 0xF2, 0xF3 }), CVEC({ 0x0F, 0xAE })) },
    { "STAC",           ZO_I_BASE(0xCB, CVEC({ 0x66, 0xF2, 0xF3 }), CVEC({ 0x0F, 0x01 })) },
    { "STC",            ZO_I_OPC(0xF9) },
    { "STD",            ZO_I_OPC(0xFD) },
    { "STI",            ZO_I_OPC(0xFB) },
    { "STOSB",          ZO_I_OPC(0xAA) },
    { "STOSW",          ZO_I_EXT(0xAB, {}, BitsMode::M32, 0x66, {}) },
    { "STOSD",          ZO_I_EXT(0xAB, {}, BitsMode::M16, 0x66, {} )},
    { "SYSENTER",       ZO_I_BASE(0x34, {}, CVEC({ 0x0F })) },
    { "SYSEXIT",        ZO_I_BASE(0x35, {}, CVEC({ 0x0F })) },
    { "UD2",            ZO_I_BASE(0x0B, {}, CVEC({ 0x0F })) },
    { "WBINVD",         ZO_I_BASE(0x09, {}, CVEC({ 0x0F })) },
    { "WBNOINVD",       ZO_I_BASE(0x09, {}, CVEC({ 0xF3, 0x0F })) },
    { "WRMSR",          ZO_I_BASE(0x30, {}, CVEC({ 0x0F })) },
    { "WRPKRU",         ZO_I_BASE(0xEF, CVEC({ 0x66, 0xF2, 0xF3 }), CVEC({ 0x0F, 0x01 })) },
    { "XGETBV",         ZO_I_BASE(0xD0, CVEC({ 0x66, 0xF2, 0xF3 }), CVEC({ 0x0F, 0x01 })) },
    { "XLATB",          ZO_I_OPC(0xD7) },
    { "XRESLDTRK",      ZO_I_BASE(0xE9, {}, CVEC({ 0xF2, 0x0F, 0x01 })) },
    { "XSETBV",         ZO_I_BASE(0xD1, CVEC({ 0x66, 0xF2, 0xF3 }), CVEC({ 0x0F, 0x01 })) },
    { "XSUSLDTRK",      ZO_I_BASE(0xE8, {}, CVEC({ 0xF2, 0x0F, 0x01 })) },
    { "XTEST",          ZO_I_BASE(0xD6, CVEC({ 0x66, 0xF2, 0xF3 }), CVEC({ 0x0F, 0x01 })) }
};

namespace {
    static bool check_forbidden_prefix(const Context& ctx, uint8_t p) {
        return std::find(
            ctx.contextual_prefixes.cbegin(),
            ctx.contextual_prefixes.cend(),
            p
        ) != ctx.contextual_prefixes.cend();
    }

    static void assemble_zo(Context& ctx, const std::string_view& instruction) {
        ZOInstruction zoi = ZOTable.at(instruction.data());

        if (!ctx.contextual_prefixes.empty() && !zoi.forbidden_prefixes.empty()) {
            for (const auto& p : zoi.forbidden_prefixes) {
                if (check_forbidden_prefix(ctx, p)) {
                    std::cerr << std::format(
                        "Error on line {}: Illegal prefix {} for instruction {}",
                        ctx.line_no,
                        p,
                        instruction
                    ) << std::endl;
                    ctx.on_error = true;
                    return;
                }
            }
        }

        ctx.output_file.write(
            (const char*)ctx.contextual_prefixes.data(),
            ctx.contextual_prefixes.size()
        );
        ctx.contextual_prefixes.clear();

        if (zoi.mode_prefix.mode != BitsMode::INVALID) {
            if (ctx.b_mode == zoi.mode_prefix.mode) {
                ctx.output_file.write((const char*)&zoi.mode_prefix.prefix, sizeof(zoi.mode_prefix.prefix));
            }
        }

        ctx.output_file.write(
            (const char*)zoi.other_prefixes.data(),
            zoi.other_prefixes.size()
        );

        ctx.output_file.write(
            (const char*)&zoi.opcode,
            sizeof(zoi.opcode)
        );
    }

    static void assemble_line(Context& ctx, const std::string_view& s) {
        if (s.empty() || s.starts_with("//") || s.starts_with(";") || s.starts_with("#")) {
            return;
        }
        else if (s.starts_with("BITS ")) {
            constexpr size_t prefix_length = 5;
            change_bits_mode(ctx, s.substr(prefix_length));
        }
        else if (s.starts_with("[BITS ")) {
            constexpr size_t prefix_length = 6;
            change_bits_mode(ctx, s.substr(prefix_length, s.size() - prefix_length - 1));
        }
        else {
            size_t delimiter_pos = s.find(" ");
            std::string_view instruction = s.substr(0, delimiter_pos);
            std::string_view args = delimiter_pos != std::string_view::npos ? s.substr(delimiter_pos + 1) : "";

            if (ZOTable.contains(instruction.data())) {
                assemble_zo(ctx, instruction);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: aus <input file> <output file>" << std::endl;
        return -1;
    }

    const char* input_file_path     = argv[1];
    const char* output_file_path    = argv[2];

    std::ifstream input_file(input_file_path);
    if (!input_file) {
        std::cerr << "Error: Could not open " << input_file_path << std::endl;
        return -1;
    }

    std::ofstream output_file(output_file_path, std::ios::binary);
    if (!output_file) {
        std::cerr << "Error: could not open " << output_file_path << std::endl;
        return -1;
    }

    Context ctx = {
        .b_mode         = M16,
        .line_no        = 1,
        .output_file    = std::move(output_file),
        .on_error       = false
    };

    std::string line;
    while (std::getline(input_file, line)) {
        size_t endpos   = line.find_last_not_of(" \t\n");
        size_t startpos = line.find_first_not_of(" \t");

        if (endpos == std::string::npos) {
            continue;
        }

        std::string normalized = line.substr(startpos, endpos - startpos + 1);

        std::transform(
            normalized.begin(),
            normalized.end(),
            normalized.begin(),
            [](unsigned char c) { return std::toupper(c); }
        );

        assemble_line(ctx, normalized);
        ++ctx.line_no;
    }

    if (ctx.on_error) {
        std::cerr << "Generation failed, output file may contain invalid data" << std::endl;
        return -1;
    }

    return 0;
}
