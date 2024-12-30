#pragma once

#include <cpu/core/core.h>
#include <cpu/riscv.h>

class RV64I : public RISCV<64> {
public:
	RV64I(const std::vector<std::string>& extensions, EmulationType type);

	void executeInstruction(std::uint32_t opcode);
};
