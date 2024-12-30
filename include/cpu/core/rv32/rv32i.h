#pragma once

#include <cpu/core/core.h>
#include <cpu/riscv.h>
#include <memory>
#include <cpu/core/backend.h>

class RV32I : public RISCV<32> {
public:
    RV32I(const std::vector<std::string>& extensions, EmulationType type);
    void step();
    void set_backend(std::unique_ptr<CoreBackend> backend);

    CoreBackend *get_backend() {
        return backend.get();
    }

private:
    void execute_opcode(std::uint32_t opcode);
    std::unique_ptr<CoreBackend> backend;
};
