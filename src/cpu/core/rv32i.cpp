#include <cpu/core/rv32i.h>
#include <cpu/registers.h>
#include <cstring>
#include <bitset>

RV32I::RV32I(const std::vector<std::string>& extensions)
		: RISCV<32>(extensions) {
	set_step_func([this] { step(); });
}

void RV32I::execute_opcode(std::uint32_t opcode) {
    std::uint8_t opcode_rv32 = 0;
    std::uint16_t opcode_ = 0;
    std::uint16_t opcode_rv16 = 0;
    std::uint8_t funct3 = 0;
	std::uint8_t funct7 = 0;

    // Check for RV16
    if ((opcode & 0x3) != 0x3) {
        opcode_ = static_cast<std::uint16_t>(opcode);
        opcode_rv16 = (opcode_ >> 0) & 0x3;
        funct3 = (opcode_rv16 >> 13) & 0x7;

        switch (opcode_rv16) {
            case 1:
                switch (funct3) {
                    case 0:
                        if (has_compressed)
                        {
                            rv32i_caddi(opcode_rv16);
                        }
                        else
                        {
                            no_ext("C");
                        }
                        break;

                    default:
                        std::ostringstream logMessage;
                        logMessage << "[RISKY] Unimplemented RV16 Opcode: 0x" << format("{:04X}", opcode_rv16) << ", funct3: 0b" << format("{:04b}", funct3);

                        Logger::Instance().Error(logMessage.str());

                        Risky::exit();
                        break;
                }
                break;

            default:
                unknown_rv16_opcode(opcode_rv16);
                break;
        }
        return;
    }
    else
    {
        // RV32
        funct3 = (opcode >> 12) & 0x7;
        funct7 = (opcode >> 25) & 0x7F;
		opcode_rv32 = opcode & 0x7F;

        switch (opcode_rv32) {

            case AUIPC:
                rv32i_auipc(opcode);
                break;

            case LUI:
                rv32i_lui(opcode);
                break;

	        case LOAD:
		        switch (funct3) {
			        case 0b010:
				        rv32i_lw(opcode);
				        break;
			        case 0b100:
				        rv32i_lbu(opcode);
				        break;
			        default:
				        unknown_load_opcode(funct3);
				        break;
		        }
		        break;

	        case MISCMEM:
                switch (funct3) {
                    case 0b001:
                        if (has_zifencei) {
                            rv32i_fence_i(opcode);
                        } else {
                            no_ext("Zifencei");
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
	                case 0b111:
		                rv32i_andi(opcode);
		                break;
                    default:
                        unknown_immediate_opcode(funct3);
                        break;
                }
                break;

            case STORE:
                switch (funct3) {
	                case 0b000:
		                rv32i_sb(opcode);
		                break;
                    case 0b010:
                        rv32i_sw(opcode);
                        break;
                    default:
                        unknown_store_opcode(funct3);
                        break;
                }
                break;

	        case OP:
		        if (has_m) {
					switch (funct7) {
						case 0b0000001:
							switch (funct3) {
								case 0b100:
									rv32i_div(opcode);
									break;
								default:
									unknown_op_opcode(funct3, funct7);
									break;
							}
							break;

					        default:
						        unknown_op_opcode(funct3, funct7);
						        break;
				        }
			        break;
				} else {
			        no_ext("M");
		        }
		        break;

            case BRANCH:
                switch (funct3) {
					case 0b000:
		                rv32i_beq(opcode);
		                break;

	                case 0b001:
						rv32i_bne(opcode);
						break;

                    case 0b100:
                        rv32i_blt(opcode);
                        break;

                    case 0b101:
                        rv32i_bge(opcode);
                        break;

					case 0b111:
		                rv32i_bgeu(opcode);
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

                        case 0b011:
                            rv32i_csrrc(opcode);
                            break;

                        case 0b101:
                            rv32i_csrrsi(opcode);
                            break;

	                    case 0b111:
		                    rv32i_csrrwi(opcode);
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
                unknown_rv32_opcode(opcode);
                break;
        }
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

void RV32I::unknown_rv16_opcode(std::uint16_t opcode) {
    std::ostringstream logMessage;
    logMessage << "[RISKY] Unimplemented RV16 Opcode: 0x" << format("{:04X}", opcode);

    Logger::Instance().Error(logMessage.str());

    Risky::exit();
}

void RV32I::unknown_rv32_opcode(std::uint32_t opcode) {
    std::ostringstream logMessage;
    logMessage << "[RISKY] Unimplemented RV32 Opcode: 0x" << format("{:08X}", opcode);

    Logger::Instance().Error(logMessage.str());

    Risky::exit();
}

void RV32I::unknown_zicsr_opcode(std::uint8_t funct3) {
	std::ostringstream logMessage;
	logMessage << "[RISKY] Unimplemented Zicsr opcode: 0b" << format("{:08b}", funct3);

	Logger::Instance().Error(logMessage.str());

	Risky::exit();
}

void RV32I::unknown_load_opcode(std::uint8_t funct3) {
	std::ostringstream logMessage;
	logMessage << "[RISKY] Unimplemented LOAD opcode: 0b" << format("{:08b}", funct3);

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

void RV32I::unknown_op_opcode(std::uint8_t funct3, std::uint8_t funct7) {
	std::ostringstream logMessage;
	logMessage << "[RISKY] Unimplemented OP opcode: funct3=0b" << std::bitset<3>(funct3)
	           << ", funct7=0b" << std::bitset<7>(funct7);

	Logger::Instance().Error(logMessage.str());

	Risky::exit();
}

void RV32I::unknown_immediate_opcode(std::uint8_t funct3) {
	std::ostringstream logMessage;
	logMessage << "[RISKY] Unimplemented OP-IMM opcode: 0b" << format("{:08b}", funct3);

	Logger::Instance().Error(logMessage.str());

	Risky::exit();
}

void RV32I::unknown_compressed_opcode(std::uint8_t funct3) {
    std::ostringstream logMessage;
    logMessage << "[RISKY] Unimplemented Compressed opcode: 0b" << format("{:08b}", funct3);

    Logger::Instance().Error(logMessage.str());

    Risky::exit();
}

// RV16
void RV32I::rv32i_caddi(std::uint16_t opcode) {
    std::uint8_t rd = ((opcode >> 7) & 0x7);
    std::uint8_t imm = ((opcode >> 2) & 0x1F);

    if (imm == 0) {
        // C.NOP
        return;
    }

    std::int32_t imm32 = static_cast<std::int32_t>(imm << 26) >> 26;
    std::int32_t result = registers[rd] + imm32;

    registers[rd] = result;
}

// AIUPC
void RV32I::rv32i_auipc(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;

	std::uint32_t imm = opcode & 0xFFFFF000;

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

	std::int32_t imm_20 = (opcode >> 31) & 0x1;
	std::int32_t imm_10_1 = (opcode >> 21) & 0x3FF;
	std::int32_t imm_11 = (opcode >> 20) & 0x1;
	std::int32_t imm_19_12 = (opcode >> 12) & 0xFF;

	std::int32_t imm = (imm_20 << 20) | (imm_19_12 << 12) | (imm_11 << 11) | (imm_10_1 << 1);

	imm = (imm << 11) >> 11;

	registers[rd] = pc + 4;

	pc += imm - 4;
}

void RV32I::rv32i_jalr(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::int32_t imm = static_cast<std::int32_t>((opcode >> 20) & 0xFFF);

	std::int32_t sign_extended_imm = (imm & (1 << 11)) ? (imm | ~((1 << 12) - 1)) : imm;

	std::int32_t temp = pc + 4;

	std::int32_t target_address = (registers[rs1] + sign_extended_imm) & ~1;

	pc = target_address - 4;

	registers[rd] = temp;
}

void RV32I::rv32i_lw(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::int32_t imm = static_cast<int32_t>(opcode) >> 20;

	uint32_t effective_address = registers[rs1] + imm;

	uint32_t loaded_value = bus.read32(effective_address);

	registers[rd] = loaded_value;
}

void RV32I::rv32i_lbu(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::int32_t imm = static_cast<int32_t>(opcode) >> 20;

	uint32_t effective_address = registers[rs1] + imm;

	uint8_t loaded_byte = bus.read8(effective_address);

	registers[rd] = static_cast<uint32_t>(static_cast<int32_t>(loaded_byte));
}

// OP
void RV32I::rv32i_div(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint8_t rs2 = (opcode >> 20) & 0x1F;

	std::int32_t dividend = static_cast<std::int32_t>(registers[rs1]);
	std::int32_t divisor = static_cast<std::int32_t>(registers[rs2]);

	if (divisor == 0) {
		registers[rd] = -1;
	} else if (dividend == INT32_MIN && divisor == -1) {
		registers[rd] = dividend;
	} else {
		registers[rd] = dividend / divisor;
	}
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

void RV32I::rv32i_csrrc(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint16_t csr = (opcode >> 20) & 0xFFF;

	std::uint32_t csr_value = csr_read(csr);
	std::uint32_t mask = registers[rs1];
	std::uint32_t cleared_value = csr_value & (~mask);

	if (rd != 0) {
		registers[rd] = cleared_value;
	}

	pc += 4;
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

void RV32I::rv32i_csrrwi(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t uimm = (opcode >> 15) & 0x1F;
	std::uint16_t csr = (opcode >> 20) & 0xFFF;

	std::uint32_t old_csr_value = csr_read(csr);

	if (rd != 0) {
		registers[rd] = old_csr_value;
	}

	csr_write(csr, uimm);
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

void RV32I::rv32i_andi(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	auto imm = static_cast<std::int32_t>(static_cast<std::int32_t>(opcode) >> 20);

	std::uint32_t result = registers[rs1] & imm;

	registers[rd] = result;
}

// BRANCH
void RV32I::rv32i_beq(std::uint32_t opcode) {
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint8_t rs2 = (opcode >> 20) & 0x1F;

	// Immediate value is spread across different parts of the instruction
	std::int32_t imm_12 = (opcode >> 31) & 0x1;
	std::int32_t imm_10_5 = (opcode >> 25) & 0x3F;
	std::int32_t imm_4_1 = (opcode >> 8) & 0xF;
	std::int32_t imm_11 = (opcode >> 7) & 0x1;

	// Combining the immediate parts and adjusting for sign extension
	std::int32_t imm = (imm_12 << 12) | (imm_11 << 11) | (imm_10_5 << 5) | (imm_4_1 << 1);
	imm = (imm << 19) >> 19;  // Sign extend to 32 bits

	if (registers[rs1] == registers[rs2]) {
		pc += imm - 4;
	}
}

void RV32I::rv32i_bne(std::uint32_t opcode) {
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint8_t rs2 = (opcode >> 20) & 0x1F;

	// Immediate value is spread across different parts of the instruction
	std::int32_t imm_12 = (opcode >> 31) & 0x1;
	std::int32_t imm_10_5 = (opcode >> 25) & 0x3F;
	std::int32_t imm_4_1 = (opcode >> 8) & 0xF;
	std::int32_t imm_11 = (opcode >> 7) & 0x1;

	// Combining the immediate parts and adjusting for sign extension
	std::int32_t imm = (imm_12 << 12) | (imm_11 << 11) | (imm_10_5 << 5) | (imm_4_1 << 1);
	imm = (imm << 19) >> 19;  // Sign extend to 32 bits

	if (registers[rs1] != registers[rs2]) {
		pc += imm - 4;
	}
}

void RV32I::rv32i_blt(std::uint32_t opcode) {
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint8_t rs2 = (opcode >> 20) & 0x1F;
	std::int32_t imm = (static_cast<std::int32_t>(opcode) >> 20);

	if (static_cast<std::int32_t>(registers[rs1]) < static_cast<std::int32_t>(registers[rs2])) {
		// Calculate the branch target address
		uint32_t target_address = pc + imm;
		// Set the PC to the branch target address
		pc = target_address;
	}
}

void RV32I::rv32i_bge(std::uint32_t opcode) {
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint8_t rs2 = (opcode >> 20) & 0x1F;

	// Immediate value is spread across different parts of the instruction
	std::int32_t imm_12 = (opcode >> 31) & 0x1;
	std::int32_t imm_10_5 = (opcode >> 25) & 0x3F;
	std::int32_t imm_4_1 = (opcode >> 8) & 0xF;
	std::int32_t imm_11 = (opcode >> 7) & 0x1;

	// Combining the immediate parts and adjusting for sign extension
	std::int32_t imm = (imm_12 << 12) | (imm_11 << 11) | (imm_10_5 << 5) | (imm_4_1 << 1);
	imm = (imm << 19) >> 19;  // Sign extend to 32 bits

	if (static_cast<std::int32_t>(registers[rs1]) >= static_cast<std::int32_t>(registers[rs2])) {
		pc += imm - 4;
	}
}

void RV32I::rv32i_bgeu(std::uint32_t opcode) {
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint8_t rs2 = (opcode >> 20) & 0x1F;

	std::int32_t imm_12 = (opcode >> 31) & 0x1;
	std::int32_t imm_10_5 = (opcode >> 25) & 0x3F;
	std::int32_t imm_4_1 = (opcode >> 8) & 0xF;
	std::int32_t imm_11 = (opcode >> 7) & 0x1;

	std::int32_t imm = (imm_12 << 12) | (imm_11 << 11) | (imm_10_5 << 5) | (imm_4_1 << 1);
	imm = (imm << 19) >> 19;

	if (static_cast<std::uint32_t>(registers[rs1]) >= static_cast<std::uint32_t>(registers[rs2])) {
		pc += imm - 4;
	}
}

// STORE
void RV32I::rv32i_sb(uint32_t opcode) {
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint8_t rs2 = (opcode >> 20) & 0x1F;

	int32_t imm_11_5 = (opcode >> 25) & 0x7F;
	int32_t imm_4_0 = (opcode >> 7) & 0x1F;
	int32_t imm = (imm_11_5 << 5) | imm_4_0;
	imm = (imm << 20) >> 20;

	uint32_t effective_address = registers[rs1] + imm;

	bus.write8(effective_address, registers[rs2] & 0xFF);
}

void RV32I::rv32i_sw(uint32_t opcode) {
	std::uint32_t imm_11_5 = (opcode >> 25) & 0x7F;
	std::uint32_t imm_4_0 = (opcode >> 7) & 0x1F;
	std::uint32_t imm = (imm_11_5 << 5) | imm_4_0;

	std::int32_t signed_imm = (imm & (1 << 11)) ? (imm | ~((1 << 12) - 1)) : imm;

	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint8_t rs2 = (opcode >> 20) & 0x1F;

	uint32_t effective_address = registers[rs1] + signed_imm;

	bus.write32(effective_address, registers[rs2]);
}
