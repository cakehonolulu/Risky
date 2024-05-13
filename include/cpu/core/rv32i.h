#pragma once

#include <cpu/riscv.h>

class RV32I : public RISCV<32> {
public:
	RV32I(const std::vector<std::string>& extensions);
    void step();
	void unknown_zicsr_opcode(std::uint8_t funct3);
	void unknown_miscmem_opcode(std::uint8_t funct3);
	void no_ext(std::string extension);

	// JAL
	void rv32i_jal(std::uint32_t opcode);

	// SYSTEM
	void rv32i_csrrw(uint32_t opcode);

private:
	void execute_opcode(std::uint32_t opcode);

	void rv32i_fence_i(uint32_t opcode);
};
