#pragma once

#include <cstdint>
#include <utility>

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
    };
};
