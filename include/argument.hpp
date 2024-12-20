#pragma once

#include <cstdint>
#include <utility>

#include "memory.hpp"
#include "registers.hpp"

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
        std::pair<MemoryOperandDescriptor, uint8_t> mem;
    };
};
