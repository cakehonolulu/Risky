#include <cpu/core/rv32i.h>
#include <cstring>

RV32I::RV32I(const std::vector<std::string>& extensions)
		: RISCV<32>(extensions) {

	set_step_func([this] { step(); });
}

void RV32I::execute_opcode(std::uint32_t opcode) {
	switch (opcode & 0x7F) {
		case JAL:
			rv32i_jal(opcode);
			break;

		default:
			unknown_opcode(opcode);
			break;
	}
}

void RV32I::step() {
	std::uint32_t opcode = fetch_opcode();
	execute_opcode(opcode);
	pc += 4;
}

void RV32I::rv32i_jal(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;

	int32_t imm =
			static_cast<int32_t> (((opcode >> 31) & 0x1) << 20 |
			                      ((opcode >> 21) & 0x3FF) << 1 |
			                      ((opcode >> 20) & 0x1)) << 11 >> 11;

	registers[rd] = pc + 4;

	pc = (pc + imm);
}
