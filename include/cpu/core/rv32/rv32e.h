#pragma once

#include <cpu/core/core.h>
#include <cpu/riscv.h>

class RV32E : public RISCV<32, EMBEDDED> {
public:
	RV32E(const std::vector<std::string>& extensions, EmulationType type);

	void executeInstruction(std::uint32_t opcode);
};
