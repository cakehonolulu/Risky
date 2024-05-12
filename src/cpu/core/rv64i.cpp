#include <cpu/core/rv64i.h>

RV64I::RV64I(const std::vector<std::string>& extensions)
		: RISCV<64>(extensions) {
	// Constructor implementation for rv64i
}

void RV64I::executeInstruction(std::uint32_t opcode) {
	// Implementation of instruction execution for rv64i
	// Example: decode opcode and execute corresponding instruction
}
