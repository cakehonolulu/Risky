#pragma once

#include <cpu/riscv.h>
#include <memory>
#include <cpu/core/backend.h>

class RV32I : public RISCV<32> {
public:
    RV32I(const std::vector<std::string>& extensions, std::unique_ptr<CoreBackend> backend);
    void step();
    void set_backend(std::unique_ptr<CoreBackend> backend);

private:
    void execute_opcode(std::uint32_t opcode);
    std::unique_ptr<CoreBackend> backend;
};
