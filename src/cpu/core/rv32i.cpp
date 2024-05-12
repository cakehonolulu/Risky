#include <cpu/core/rv32i.h>

RV32I::RV32I(const std::vector<std::string>& extensions)
		: RISCV<32>(extensions) {

	set_step_func([this] { step(); });
}

void RV32I::execute_opcode(std::uint32_t opcode) {
	switch (opcode) {
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
