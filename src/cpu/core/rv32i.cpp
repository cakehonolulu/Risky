#include <cpu/core/rv32i.h>
#include <cstring>

RV32I::RV32I(const std::vector<std::string>& extensions)
		: RISCV<32>(extensions) {

	set_step_func([this] { step(); });


}

void RV32I::execute_opcode(std::uint32_t opcode) {
	std::uint8_t opcode_ = opcode & 0x7F;
	std::uint8_t funct3 = (opcode >> 12) & 0x7;
	switch (opcode_) {
		case JAL:
			rv32i_jal(opcode);
			break;

		case SYSTEM:
			if (has_zicsr)
			{
				switch (funct3) {
					case 0b001:
						rv32i_csrrw(opcode);
						break;

					default:
						unknown_zicsr_opcode(funct3);
						break;
				}
			}
			else
			{
				no_ext("Zicsr");
			}

			break;

		case MISCMEM:
			switch (funct3) {
				case 0b001:
					if (has_zifence) {
						rv32i_fence_i(opcode);
					} else {
						no_ext("Zifence");
					}
					break;
				default:
					unknown_miscmem_opcode(funct3);
					break;
			}
			break;

		default:
			unknown_opcode(opcode);
			break;
	}
}

void RV32I::step() {
	std::uint32_t opcode = fetch_opcode();
	execute_opcode(opcode);
	registers[0] = 0;
	pc += 4;
}

void RV32I::no_ext(std::string extension) {
	std::ostringstream logMessage;
	logMessage << "[RISKY] FATAL ERROR: Called a " << extension.c_str() << " extension opcode but it's unavailable for this core!";

	Logger::Instance().Error(logMessage.str());

	Risky::exit();
}

void RV32I::unknown_zicsr_opcode(std::uint8_t funct3) {
	std::ostringstream logMessage;
	logMessage << "[RISKY] Unimplemented Zicsr opcode: 0b" << format("{:08b}", funct3);

	Logger::Instance().Error(logMessage.str());

	Risky::exit();
}

void RV32I::unknown_miscmem_opcode(std::uint8_t funct3) {
	std::ostringstream logMessage;
	logMessage << "[RISKY] Unimplemented MISC-MEM opcode: 0b" << format("{:08b}", funct3);

	Logger::Instance().Error(logMessage.str());

	Risky::exit();
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

void RV32I::rv32i_csrrw(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint16_t csr = (opcode >> 20) & 0xFFF;
	registers[rd] = csr_read(csr);
	csr_write(csr, registers[rs1]);
}

void RV32I::rv32i_fence_i(uint32_t opcode) {
	/*
	 * TODO:
	 * In case we have different harts, we need to implement this;
	 * since we're currently defaulting to only using 1 hart (Hart #0),
	 * this should have pretty much no effect considering memory
	 * reads/writes are instant and there's no delay neither bus penalty/stall.
	 */
	return;
}
