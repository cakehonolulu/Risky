#include <cpu/core/rv32/rv32i.h>
#include <cpu/core/rv32/backends/rv32i_interpreter.h>
#include <cpu/core/rv32/backends/rv32i_jit.h>
#include <cpu/registers.h>
#include <cstring>
#include <bitset>
#include <cpu/core/core.h>

RV32I::RV32I(const std::vector<std::string>& extensions, EmulationType type)
    : RISCV<32>(extensions), backend(std::move(backend)) {
    if (type == EmulationType::JIT) {
        backend = std::make_unique<RV32IJIT>(this);
    } else {
        backend = std::make_unique<RV32IInterpreter>(this);
    }
    
    set_step_func([this] { backend->step(); });
    set_run_func([this] { backend->run(); });
}

void RV32I::execute_opcode(std::uint32_t opcode) {
    backend->execute_opcode(opcode);
}

void RV32I::set_backend(std::unique_ptr<CoreBackend> backend) {
    this->backend = std::move(backend);
}
