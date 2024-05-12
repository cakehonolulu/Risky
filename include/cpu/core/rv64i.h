#pragma once

#include <cpu/riscv.h>

class RV64I : public RISCV<64> {
public:
	RV64I(const std::vector<std::string>& extensions);

	void executeInstruction(std::uint32_t opcode);
};
