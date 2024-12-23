#include <cpu/core/rv32/rv32i.h>
#include <cpu/core/rv32/backends/rv32i_interpreter.h>
#include <cpu/core/rv32/backends/rv32i_jit.h>
#include <cpu/registers.h>
#include <cstring>
#include <bitset>
#include <cpu/core/core.h>

RV32I::RV32I(const std::vector<std::string>& extensions, std::unique_ptr<CoreBackend> backend)
    : RISCV<32>(extensions), backend(std::move(backend)) {
    set_step_func([this] { step(); });
}

void RV32I::step() {
    std::uint32_t opcode = fetch_opcode();
    execute_opcode(opcode);
    registers[0] = 0;
    pc += 4;
}

void RV32I::execute_opcode(std::uint32_t opcode) {
    backend->execute_opcode(opcode);
}

void RV32I::set_backend(std::unique_ptr<CoreBackend> backend) {
    this->backend = std::move(backend);
}
