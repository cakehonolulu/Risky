#pragma once

#include <cpu/riscv.h>

class RV32E : public RISCV<32, EMBEDDED> {
public:
	RV32E(const std::vector<std::string>& extensions);

	void executeInstruction(std::uint32_t opcode);
};
