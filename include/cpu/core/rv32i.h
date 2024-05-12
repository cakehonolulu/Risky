#pragma once

#include <cpu/riscv.h>

class RV32I : public RISCV<32> {
public:
	RV32I(const std::vector<std::string>& extensions);

private:
	void execute_opcode(std::uint32_t opcode);
	void step();
};
