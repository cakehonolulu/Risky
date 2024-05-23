#pragma once

#include <cpu/riscv.h>

class RV32I : public RISCV<32> {
public:
	RV32I(const std::vector<std::string>& extensions);
    void step();
    void unknown_rv16_opcode(std::uint16_t opcode);
    void unknown_rv32_opcode(std::uint32_t opcode);
	void unknown_zicsr_opcode(std::uint8_t funct3);
	void unknown_miscmem_opcode(std::uint8_t funct3);
	void unknown_branch_opcode(std::uint8_t funct3);
	void unknown_store_opcode(std::uint8_t funct3);
	void unknown_immediate_opcode(std::uint8_t funct3);
    void unknown_compressed_opcode(std::uint8_t funct3);
	void no_ext(std::string extension);


private:
	void execute_opcode(std::uint32_t opcode);

    // RV16
    void rv32i_caddi(std::uint16_t opcode);

	// AUIPC
	void rv32i_auipc(std::uint32_t opcode);

	// LUI
	void rv32i_lui(std::uint32_t opcode);

	// JAL
	void rv32i_jal(std::uint32_t opcode);

	// JALR
	void rv32i_jalr(std::uint32_t opcode);

	// MISC-MEM
	void rv32i_fence_i(std::uint32_t opcode);

	// OP-IMM
	void rv32i_addi(std::uint32_t opcode);

	// BRANCH
	void rv32i_bne(std::uint32_t opcode);
	void rv32i_blt(std::uint32_t opcode);
	void rv32i_bge(std::uint32_t opcode);

	// STORE
	void rv32i_sw(std::uint32_t opcode);

	// SYSTEM
	void rv32i_csrrw(std::uint32_t opcode);
	void rv32i_csrrs(std::uint32_t opcode);
	void rv32i_csrrc(std::uint32_t opcode);
	void rv32i_csrrsi(std::uint32_t opcode);
};
