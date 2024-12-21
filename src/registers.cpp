#include <cstdint>
#include <string_view>
#include <unordered_map>

#include "registers.hpp"

std::unordered_map<std::string_view, std::pair<AsmRegister, int32_t>> REGISTERS = {
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

std::unordered_map<AsmRegister, uint8_t> REGISTERS_ENCODING = {
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
