#include <cpu/core/rv32/backends/rv32i_jit.h>
#include <cpu/core/rv32/rv32i.h>
#include <log/log.hh>
#include <risky.h>
#include <cpu/core/core.h>

RV32IJIT::RV32IJIT(RV32I* core) : core(core) {
    Logger::error("JIT execution not implemented yet.");
    Risky::exit(1, Risky::Subsystem::Core);
}

void RV32IJIT::execute_opcode(std::uint32_t opcode) {
}
