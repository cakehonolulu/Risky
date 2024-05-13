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

		case AUIPC:
			rv32i_auipc(opcode);
			break;

		case LUI:
			rv32i_lui(opcode);
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

		case OPIMM:
			switch (funct3) {
				case 0b000:
					rv32i_addi(opcode);
					break;
				default:
					unknown_immediate_opcode(funct3);
					break;
			}
			break;

		case STORE:
			switch (funct3) {
				case 0b010:
					rv32i_sw(opcode);
					break;
				default:
					unknown_store_opcode(funct3);
					break;
			}
			break;

		case BRANCH:
			switch (funct3) {
				case 0b101:
					rv32i_bge(opcode);
					break;
				default:
					unknown_branch_opcode(funct3);
					break;
			}
			break;

		case JAL:
			rv32i_jal(opcode);
			break;

		case JALR:
			rv32i_jalr(opcode);
			break;

		case SYSTEM:
			if (has_zicsr)
			{
				switch (funct3) {
					case 0b001:
						rv32i_csrrw(opcode);
						break;

					case 0b010:
						rv32i_csrrs(opcode);
						break;

					case 0b101:
						rv32i_csrrsi(opcode);
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

void RV32I::unknown_branch_opcode(std::uint8_t funct3) {
	std::ostringstream logMessage;
	logMessage << "[RISKY] Unimplemented BRANCH opcode: 0b" << format("{:08b}", funct3);

	Logger::Instance().Error(logMessage.str());

	Risky::exit();
}

void RV32I::unknown_store_opcode(std::uint8_t funct3) {
	std::ostringstream logMessage;
	logMessage << "[RISKY] Unimplemented STORE opcode: 0b" << format("{:08b}", funct3);

	Logger::Instance().Error(logMessage.str());

	Risky::exit();
}

void RV32I::unknown_immediate_opcode(std::uint8_t funct3) {
	std::ostringstream logMessage;
	logMessage << "[RISKY] Unimplemented OP-IMM opcode: 0b" << format("{:08b}", funct3);

	Logger::Instance().Error(logMessage.str());

	Risky::exit();
}

void RV32I::rv32i_auipc(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::int32_t imm = static_cast<std::int32_t>((opcode & 0xFFFFF000) >> 12);

	std::uint32_t result = pc + imm;

	registers[rd] = result;
}

void RV32I::rv32i_lui(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::int32_t imm = static_cast<std::int32_t>((opcode & 0xFFFFF000) >> 12);

	std::uint32_t result = static_cast<std::uint32_t>(imm) << 12;

	registers[rd] = result;
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

void RV32I::rv32i_jalr(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::int32_t imm = static_cast<std::int32_t>((opcode >> 20) & 0xFFF);

	std::int32_t target_address = (registers[rs1] + imm) & ~1;

	registers[rd] = pc + 4;
	pc = target_address;
}

void RV32I::rv32i_csrrw(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint16_t csr = (opcode >> 20) & 0xFFF;
	registers[rd] = csr_read(csr);
	csr_write(csr, registers[rs1]);
}

void RV32I::rv32i_csrrs(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint16_t csr = (opcode >> 20) & 0xFFF;

	std::uint32_t csr_value = csr_read(csr);
	std::uint32_t bit_mask = registers[rs1];

	csr_value |= bit_mask;
	registers[rd] = csr_value;
}

void RV32I::rv32i_csrrsi(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint16_t csr = (opcode >> 20) & 0xFFF;
	std::uint8_t uimm = rs1;

	std::uint32_t old_csr_value = csr_read(csr);
	std::uint32_t csr_value = old_csr_value & 0xFFFFFFFF;
	std::uint32_t mask = (1U << 5) - 1;

	if (rs1 != 0 && (uimm & mask) != 0) {
		csr_value |= (uimm & mask);
	}

	csr_write(csr, csr_value);

	if (rd != 0) {
		registers[rd] = old_csr_value;
	}
}

void RV32I::rv32i_fence_i(std::uint32_t opcode) {
	/*
	 * TODO:
	 * In case we have different harts, we need to implement this;
	 * since we're currently defaulting to only using 1 hart (Hart #0),
	 * this should have pretty much no effect considering memory
	 * reads/writes are instant and there's no delay neither bus penalty/stall.
	 */
	return;
}

void RV32I::rv32i_addi(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	auto imm = static_cast<std::int32_t>(static_cast<std::int32_t>(opcode) >> 20);

	registers[rd] = registers[rs1] + imm;
}

// BRANCH
void RV32I::rv32i_bge(std::uint32_t opcode) {
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint8_t rs2 = (opcode >> 20) & 0x1F;
	std::int32_t imm = static_cast<std::int32_t>((opcode & 0xFFF00000) >> 20);

	if (static_cast<std::int32_t>(registers[rs1]) >= static_cast<std::int32_t>(registers[rs2])) {
		pc += imm;
	} else {
		pc += 4;
	}
}

// STORE
void RV32I::rv32i_sw(uint32_t opcode) {
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint8_t rs2 = (opcode >> 20) & 0x1F;
	std::int32_t imm = (static_cast<std::int32_t>(opcode) >> 20);

	uint32_t effective_address = registers[rs1] + imm;
	bus.write32(effective_address, registers[rs2]);
}

