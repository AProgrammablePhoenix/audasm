#pragma once

#include <cstdint>
#include <string_view>
#include <unordered_map>

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

extern std::unordered_map<std::string_view, std::pair<AsmRegister, int32_t>> REGISTERS;
extern std::unordered_map<AsmRegister, uint8_t> REGISTERS_ENCODING;
