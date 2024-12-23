#pragma once

#include <cpu/core/core.h>
#include <cstdint>

class RV32I;

class RV32IJIT : public CoreBackend {
public:
    RV32IJIT(RV32I* core);
    void execute_opcode(std::uint32_t opcode) override;

private:
    RV32I* core;
};
