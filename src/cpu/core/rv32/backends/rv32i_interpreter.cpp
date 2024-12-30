#include <cpu/core/rv32/backends/rv32i_interpreter.h>
#include <cpu/core/rv32/rv32i.h>
#include <log/log.hh>
#include <risky.h>
#include <sstream>
#include <bitset>
#include <cpu/core/core.h>

RV32IInterpreter::RV32IInterpreter(RV32I* core) : core(core) {
    ready = true;
}

void RV32IInterpreter::execute_opcode(std::uint32_t opcode) {
    std::uint8_t opcode_rv32 = 0;
    std::uint16_t opcode_ = 0;
    std::uint16_t opcode_rv16 = 0;
    std::uint8_t funct3 = 0;
    std::uint32_t funct7 = 0;

    // Check for RV16
    if ((opcode & 0x3) != 0x3) {
        opcode_ = static_cast<std::uint16_t>(opcode);
        opcode_rv16 = (opcode_ >> 0) & 0x3;
        funct3 = (opcode_rv16 >> 13) & 0x7;

        switch (opcode_rv16) {
            case 1:
                switch (funct3) {
                    case 0:
                        if (core->has_compressed)
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
                        logMessage << "Unimplemented RV16 Opcode: 0x" << format("{:04X}", opcode_rv16) << ", funct3: 0b" << format("{:04b}", funct3);

                        Logger::error(logMessage.str());

                        Risky::exit(1, Risky::Subsystem::Core);
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
                    case 0b000:
                        rv32i_lb(opcode);
                        break;
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
                        if (core->has_zifencei) {
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
                    case 0b001:
                        rv32i_slli(opcode);
                        break;
                    case 0b011:
                        rv32i_sltiu(opcode);
                        break;
                    case 0b101:
                        if (funct7 & (1 << 5)) {
                            // SRAI
                        } else {
                            rv32i_srli(opcode);
                        }
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

            case AMO:
                switch (funct3) {
                    case 0b010:
                        switch (opcode >> 27) {
                            case 0b0000000:
                                rv32i_amoadd_w(opcode);
                                break;
                            case 0b0001000:
                                rv32i_amoor_w(opcode);
                                break;
                            default:
                                unknown_amo_opcode(funct3, funct7);
                                break;
                        }
                        break;
                    default:
                        unknown_amo_opcode(funct3, funct7);
                        break;
                }
                break;

            case OP:
                if ((funct7 & (1 << 0)) == 0) {
                    switch (funct3) {
                        case 0b000:
                        {
                            if (funct7 == 0x20) {
                                rv32i_sub(opcode);
                            } else if (funct7 == 0x00) {
                                rv32i_add(opcode);
                            }
                        }
                        break;

                        case 0b001:
                            rv32i_sll(opcode);
                            break;

                        case 0b011:
                            rv32i_sltu(opcode);
                            break;

                        case 0b100:
                            rv32i_xor(opcode);
                            break;

                        case 0b110:
                            rv32i_or(opcode);
                            break;

                        case 0b111:
                            rv32i_and(opcode);
                            break;

                        default:
                            unknown_op_opcode(funct3, funct7);
                            break;
                    }
                }
                else
                {
                    switch (funct3) {
                        case 0b000:
                            if (core->has_m) {
                                rv32i_mul(opcode);
                            } else {
                                no_ext("M");
                            }
                            break;

                        case 0b011:
                            if (core->has_m) {
                                rv32i_mulhu(opcode);
                            } else {
                                no_ext("M");
                            }
                            break;

                        case 0b100:
                            if (core->has_m) {
                                rv32i_div(opcode);
                            } else {
                                no_ext("M");
                            }
                            break;
                        default:
                            unknown_op_opcode(funct3, funct7);
                            break;
                    }
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

                    case 0b110:
                        rv32i_bltu(opcode);
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
                if (core->has_zicsr)
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

void RV32IInterpreter::no_ext(std::string extension) {
	std::ostringstream logMessage;
	logMessage << "FATAL ERROR: Called a " << extension.c_str() << " extension opcode but it's unavailable for this core!";

	Logger::error(logMessage.str());

	Risky::exit(1, Risky::Subsystem::Core);
}

void RV32IInterpreter::unknown_rv16_opcode(std::uint16_t opcode) {
    std::ostringstream logMessage;
    logMessage << "Unimplemented RV16 Opcode: 0x" << format("{:04X}", opcode);

    Logger::error(logMessage.str());

    Risky::exit(1, Risky::Subsystem::Core);
}

void RV32IInterpreter::unknown_rv32_opcode(std::uint32_t opcode) {
    std::ostringstream logMessage;
    logMessage << "Unimplemented RV32 Opcode: 0x" << format("{:08X}", opcode);

    Logger::error(logMessage.str());

    Risky::exit(1, Risky::Subsystem::Core);
}

void RV32IInterpreter::unknown_zicsr_opcode(std::uint8_t funct3) {
	std::ostringstream logMessage;
	logMessage << "Unimplemented Zicsr opcode: 0b" << format("{:08b}", funct3);

	Logger::error(logMessage.str());

	Risky::exit(1, Risky::Subsystem::Core);
}

void RV32IInterpreter::unknown_load_opcode(std::uint8_t funct3) {
	std::ostringstream logMessage;
	logMessage << "Unimplemented LOAD opcode: 0b" << format("{:08b}", funct3);

	Logger::error(logMessage.str());

	Risky::exit(1, Risky::Subsystem::Core);
}

void RV32IInterpreter::unknown_miscmem_opcode(std::uint8_t funct3) {
	std::ostringstream logMessage;
	logMessage << "Unimplemented MISC-MEM opcode: 0b" << format("{:08b}", funct3);

	Logger::error(logMessage.str());

	Risky::exit(1, Risky::Subsystem::Core);
}

void RV32IInterpreter::unknown_branch_opcode(std::uint8_t funct3) {
	std::ostringstream logMessage;
	logMessage << "Unimplemented BRANCH opcode: 0b" << format("{:08b}", funct3);

	Logger::error(logMessage.str());

	Risky::exit(1, Risky::Subsystem::Core);
}

void RV32IInterpreter::unknown_store_opcode(std::uint8_t funct3) {
	std::ostringstream logMessage;
	logMessage << "Unimplemented STORE opcode: 0b" << format("{:08b}", funct3);

	Logger::error(logMessage.str());

	Risky::exit(1, Risky::Subsystem::Core);
}

void RV32IInterpreter::unknown_amo_opcode(std::uint8_t funct3, std::uint8_t funct7) {
    std::ostringstream logMessage;
    logMessage << "Unimplemented AMO opcode: funct3=0b" << std::bitset<3>(funct3)
               << ", funct7=0b" << std::bitset<7>(funct7);

    Logger::error(logMessage.str());

    Risky::exit(1, Risky::Subsystem::Core);
}

void RV32IInterpreter::unknown_op_opcode(std::uint8_t funct3, std::uint8_t funct7) {
	std::ostringstream logMessage;
	logMessage << "Unimplemented OP opcode: funct3=0b" << std::bitset<3>(funct3)
	           << ", funct7=0b" << std::bitset<7>(funct7);

	Logger::error(logMessage.str());

	Risky::exit(1, Risky::Subsystem::Core);
}

void RV32IInterpreter::unknown_immediate_opcode(std::uint8_t funct3) {
	std::ostringstream logMessage;
	logMessage << "Unimplemented OP-IMM opcode: 0b" << format("{:08b}", funct3);

	Logger::error(logMessage.str());

	Risky::exit(1, Risky::Subsystem::Core);
}

void RV32IInterpreter::unknown_compressed_opcode(std::uint8_t funct3) {
    std::ostringstream logMessage;
    logMessage << "Unimplemented Compressed opcode: 0b" << format("{:08b}", funct3);

    Logger::error(logMessage.str());

    Risky::exit(1, Risky::Subsystem::Core);
}

// RV16
void RV32IInterpreter::rv32i_caddi(std::uint16_t opcode) {
    std::uint8_t rd = ((opcode >> 7) & 0x7);
    std::uint8_t imm = ((opcode >> 2) & 0x1F);

    if (imm == 0) {
        // C.NOP
        return;
    }

    std::int32_t imm32 = static_cast<std::int32_t>(imm << 26) >> 26;
    std::int32_t result = core->registers[rd] + imm32;

    core->registers[rd] = result;
}

// AIUPC
void RV32IInterpreter::rv32i_auipc(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;

	std::uint32_t imm = opcode & 0xFFFFF000;

	std::uint32_t result = core->pc + imm;

	core->registers[rd] = result;
}

void RV32IInterpreter::rv32i_lui(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::int32_t imm = static_cast<std::int32_t>((opcode & 0xFFFFF000) >> 12);

	std::uint32_t result = static_cast<std::uint32_t>(imm) << 12;

	core->registers[rd] = result;
}

void RV32IInterpreter::rv32i_jal(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;

	std::int32_t imm_20 = (opcode >> 31) & 0x1;
	std::int32_t imm_10_1 = (opcode >> 21) & 0x3FF;
	std::int32_t imm_11 = (opcode >> 20) & 0x1;
	std::int32_t imm_19_12 = (opcode >> 12) & 0xFF;

	std::int32_t imm = (imm_20 << 20) | (imm_19_12 << 12) | (imm_11 << 11) | (imm_10_1 << 1);

	imm = (imm << 11) >> 11;

	core->registers[rd] = core->pc + 4;

	core->pc += imm - 4;
}

void RV32IInterpreter::rv32i_jalr(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::int32_t imm = static_cast<std::int32_t>((opcode >> 20) & 0xFFF);

	std::int32_t sign_extended_imm = (imm & (1 << 11)) ? (imm | ~((1 << 12) - 1)) : imm;

	std::int32_t temp = core->pc + 4;

	std::int32_t target_address = (core->registers[rs1] + sign_extended_imm) & ~1;

	core->pc = target_address - 4;

	core->registers[rd] = temp;
}

void RV32IInterpreter::rv32i_lb(std::uint32_t opcode) {
    std::uint8_t rd = (opcode >> 7) & 0x1F;
    std::uint8_t rs1 = (opcode >> 15) & 0x1F;
    std::int32_t imm = static_cast<int32_t>(opcode) >> 20;

    uint32_t effective_address = core->registers[rs1] + imm;

    int8_t loaded_value = core->bus.read8(effective_address);

    std::int32_t sign_extended_value = static_cast<std::int32_t>(loaded_value);

    core->registers[rd] = sign_extended_value;
}

void RV32IInterpreter::rv32i_lw(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::int32_t imm = static_cast<int32_t>(opcode) >> 20;

	uint32_t effective_address = core->registers[rs1] + imm;

	uint32_t loaded_value = core->bus.read32(effective_address);

	core->registers[rd] = loaded_value;
}

void RV32IInterpreter::rv32i_lbu(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::int32_t imm = static_cast<int32_t>(opcode) >> 20;

	uint32_t effective_address = core->registers[rs1] + imm;

	uint8_t loaded_byte = core->bus.read8(effective_address);

	core->registers[rd] = static_cast<uint32_t>(static_cast<int32_t>(loaded_byte));
}

// OP
void RV32IInterpreter::rv32i_sub(std::uint32_t opcode) {
    std::uint8_t rd = (opcode >> 7) & 0x1F;
    std::uint8_t rs1 = (opcode >> 15) & 0x1F;
    std::uint8_t rs2 = (opcode >> 20) & 0x1F;

    std::uint32_t result = core->registers[rs1] - core->registers[rs2];

    core->registers[rd] = result;
}

void RV32IInterpreter::rv32i_add(std::uint32_t opcode) {
    std::uint8_t rd = (opcode >> 7) & 0x1F;
    std::uint8_t rs1 = (opcode >> 15) & 0x1F;
    std::uint8_t rs2 = (opcode >> 20) & 0x1F;

    std::uint32_t result = core->registers[rs1] + core->registers[rs2];

    core->registers[rd] = result;
}

void RV32IInterpreter::rv32i_sll(std::uint32_t opcode) {
    std::uint8_t rd = (opcode >> 7) & 0x1F;
    std::uint8_t rs1 = (opcode >> 15) & 0x1F;
    std::uint8_t rs2 = (opcode >> 20) & 0x1F;

    std::uint32_t result = core->registers[rs1] << (core->registers[rs2] & 0x1F);

    core->registers[rd] = result;
}

void RV32IInterpreter::rv32i_sltu(std::uint32_t opcode) {
    std::uint8_t rd = (opcode >> 7) & 0x1F;
    std::uint8_t rs1 = (opcode >> 15) & 0x1F;
    std::uint8_t rs2 = (opcode >> 20) & 0x1F;

    std::uint32_t result = (core->registers[rs1] < core->registers[rs2]) ? 1 : 0;

    core->registers[rd] = result;
}

void RV32IInterpreter::rv32i_xor(std::uint32_t opcode) {
    std::uint8_t rd = (opcode >> 7) & 0x1F;
    std::uint8_t rs1 = (opcode >> 15) & 0x1F;
    std::uint8_t rs2 = (opcode >> 20) & 0x1F;

    std::uint32_t result = core->registers[rs1] ^ core->registers[rs2];

    core->registers[rd] = result;
}

void RV32IInterpreter::rv32i_or(std::uint32_t opcode) {
    std::uint8_t rd = (opcode >> 7) & 0x1F;
    std::uint8_t rs1 = (opcode >> 15) & 0x1F;
    std::uint8_t rs2 = (opcode >> 20) & 0x1F;

    std::uint32_t result = core->registers[rs1] | core->registers[rs2];

    core->registers[rd] = result;
}

void RV32IInterpreter::rv32i_and(std::uint32_t opcode) {
    std::uint8_t rd = (opcode >> 7) & 0x1F;
    std::uint8_t rs1 = (opcode >> 15) & 0x1F;
    std::uint8_t rs2 = (opcode >> 20) & 0x1F;

    std::uint32_t result = core->registers[rs1] & core->registers[rs2];

    core->registers[rd] = result;
}

void RV32IInterpreter::rv32i_mul(std::uint32_t opcode) {
    std::uint8_t rd = (opcode >> 7) & 0x1F;
    std::uint8_t rs1 = (opcode >> 15) & 0x1F;
    std::uint8_t rs2 = (opcode >> 20) & 0x1F;

    std::uint32_t result = core->registers[rs1] * core->registers[rs2];

    core->registers[rd] = result;
}

void RV32IInterpreter::rv32i_mulhu(std::uint32_t opcode) {
    std::uint8_t rd = (opcode >> 7) & 0x1F;
    std::uint8_t rs1 = (opcode >> 15) & 0x1F;
    std::uint8_t rs2 = (opcode >> 20) & 0x1F;

    std::uint64_t result = static_cast<std::uint64_t>(core->registers[rs1]) * static_cast<std::uint64_t>(core->registers[rs2]);

    core->registers[rd] = static_cast<std::uint32_t>((result >> 32) & 0xFFFFFFFF);
}

void RV32IInterpreter::rv32i_div(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint8_t rs2 = (opcode >> 20) & 0x1F;

	std::int32_t dividend = static_cast<std::int32_t>(core->registers[rs1]);
	std::int32_t divisor = static_cast<std::int32_t>(core->registers[rs2]);

	if (divisor == 0) {
		core->registers[rd] = -1;
	} else if (dividend == INT32_MIN && divisor == -1) {
		core->registers[rd] = dividend;
	} else {
		core->registers[rd] = dividend / divisor;
	}
}

void RV32IInterpreter::rv32i_csrrw(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint16_t csr = (opcode >> 20) & 0xFFF;
	core->registers[rd] = core->csr_read(csr);
	core->csr_write(csr, core->registers[rs1]);
}

void RV32IInterpreter::rv32i_csrrs(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint16_t csr = (opcode >> 20) & 0xFFF;

	std::uint32_t csr_value = core->csr_read(csr);
	std::uint32_t bit_mask = core->registers[rs1];

	csr_value |= bit_mask;
	core->registers[rd] = csr_value;
}

void RV32IInterpreter::rv32i_csrrc(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint16_t csr = (opcode >> 20) & 0xFFF;

	std::uint32_t csr_value = core->csr_read(csr);
	std::uint32_t mask = core->registers[rs1];
	std::uint32_t cleared_value = csr_value & (~mask);

	if (rd != 0) {
		core->registers[rd] = cleared_value;
	}

	core->pc += 4;
}

void RV32IInterpreter::rv32i_csrrsi(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint16_t csr = (opcode >> 20) & 0xFFF;
	std::uint8_t uimm = rs1;

	std::uint32_t old_csr_value = core->csr_read(csr);
	std::uint32_t csr_value = old_csr_value & 0xFFFFFFFF;
	std::uint32_t mask = (1U << 5) - 1;

	if (rs1 != 0 && (uimm & mask) != 0) {
		csr_value |= (uimm & mask);
	}

	core->csr_write(csr, csr_value);

	if (rd != 0) {
		core->registers[rd] = old_csr_value;
	}
}

void RV32IInterpreter::rv32i_csrrwi(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t uimm = (opcode >> 15) & 0x1F;
	std::uint16_t csr = (opcode >> 20) & 0xFFF;

	std::uint32_t old_csr_value = core->csr_read(csr);

	if (rd != 0) {
		core->registers[rd] = old_csr_value;
	}

	core->csr_write(csr, uimm);
}

void RV32IInterpreter::rv32i_fence_i(std::uint32_t opcode) {
	/*
	 * TODO:
	 * In case we have different harts, we need to implement this;
	 * since we're currently defaulting to only using 1 hart (Hart #0),
	 * this should have pretty much no effect considering memory
	 * reads/writes are instant and there's no delay neither bus penalty/stall.
	 */
	return;
}

void RV32IInterpreter::rv32i_addi(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	auto imm = static_cast<std::int32_t>(static_cast<std::int32_t>(opcode) >> 20);

	core->registers[rd] = core->registers[rs1] + imm;
}

void RV32IInterpreter::rv32i_slli(std::uint32_t opcode) {
    std::uint8_t rd = (opcode >> 7) & 0x1F;
    std::uint8_t rs1 = (opcode >> 15) & 0x1F;
    std::uint8_t shamt = (opcode >> 20) & 0x1F;

    std::uint32_t result = core->registers[rs1] << shamt;

    core->registers[rd] = result;
}

void RV32IInterpreter::rv32i_sltiu(std::uint32_t opcode) {
    std::uint8_t rd = (opcode >> 7) & 0x1F;
    std::uint8_t rs1 = (opcode >> 15) & 0x1F;
    auto imm = static_cast<std::uint32_t>(opcode) >> 20;

    std::uint32_t result = (core->registers[rs1] < imm) ? 1 : 0;

    core->registers[rd] = result;
}

void RV32IInterpreter::rv32i_srli(std::uint32_t opcode) {
    std::uint8_t rd = (opcode >> 7) & 0x1F;
    std::uint8_t rs1 = (opcode >> 15) & 0x1F;
    std::uint8_t shamt = (opcode >> 20) & 0x1F;

    std::uint32_t result = core->registers[rs1] >> shamt;

    core->registers[rd] = result;
}

void RV32IInterpreter::rv32i_andi(std::uint32_t opcode) {
	std::uint8_t rd = (opcode >> 7) & 0x1F;
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	auto imm = static_cast<std::int32_t>(static_cast<std::int32_t>(opcode) >> 20);

	std::uint32_t result = core->registers[rs1] & imm;

	core->registers[rd] = result;
}

// BRANCH
void RV32IInterpreter::rv32i_beq(std::uint32_t opcode) {
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

	if (core->registers[rs1] == core->registers[rs2]) {
		core->pc += imm - 4;
	}
}

void RV32IInterpreter::rv32i_bne(std::uint32_t opcode) {
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

	if (core->registers[rs1] != core->registers[rs2]) {
		core->pc += imm - 4;
	}
}

void RV32IInterpreter::rv32i_blt(std::uint32_t opcode) {
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint8_t rs2 = (opcode >> 20) & 0x1F;
	std::int32_t imm = (static_cast<std::int32_t>(opcode) >> 20);

	if (static_cast<std::int32_t>(core->registers[rs1]) < static_cast<std::int32_t>(core->registers[rs2])) {
		// Calculate the branch target address
		uint32_t target_address = core->pc + imm;
		// Set the PC to the branch target address
		core->pc = target_address;
	}
}

void RV32IInterpreter::rv32i_bge(std::uint32_t opcode) {
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

	if (static_cast<std::int32_t>(core->registers[rs1]) >= static_cast<std::int32_t>(core->registers[rs2])) {
		core->pc += imm - 4;
	}
}

void RV32IInterpreter::rv32i_bltu(std::uint32_t opcode) {
    std::uint8_t rs1 = (opcode >> 15) & 0x1F;
    std::uint8_t rs2 = (opcode >> 20) & 0x1F;
    std::int32_t imm = static_cast<std::int32_t>(opcode) >> 20;

    if (core->registers[rs1] < core->registers[rs2]) {
        // Calculate the branch target address
        std::uint32_t target_address = core->pc + imm;
        // Set the PC to the branch target address
        core->pc = target_address;
    }
}

void RV32IInterpreter::rv32i_bgeu(std::uint32_t opcode) {
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint8_t rs2 = (opcode >> 20) & 0x1F;

	std::int32_t imm_12 = (opcode >> 31) & 0x1;
	std::int32_t imm_10_5 = (opcode >> 25) & 0x3F;
	std::int32_t imm_4_1 = (opcode >> 8) & 0xF;
	std::int32_t imm_11 = (opcode >> 7) & 0x1;

	std::int32_t imm = (imm_12 << 12) | (imm_11 << 11) | (imm_10_5 << 5) | (imm_4_1 << 1);
	imm = (imm << 19) >> 19;

	if (static_cast<std::uint32_t>(core->registers[rs1]) >= static_cast<std::uint32_t>(core->registers[rs2])) {
		core->pc += imm - 4;
	}
}

// STORE
void RV32IInterpreter::rv32i_sb(uint32_t opcode) {
	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint8_t rs2 = (opcode >> 20) & 0x1F;

	int32_t imm_11_5 = (opcode >> 25) & 0x7F;
	int32_t imm_4_0 = (opcode >> 7) & 0x1F;
	int32_t imm = (imm_11_5 << 5) | imm_4_0;
	imm = (imm << 20) >> 20;

	uint32_t effective_address = core->registers[rs1] + imm;

	core->bus.write8(effective_address, core->registers[rs2] & 0xFF);
}

void RV32IInterpreter::rv32i_sw(uint32_t opcode) {
	std::uint32_t imm_11_5 = (opcode >> 25) & 0x7F;
	std::uint32_t imm_4_0 = (opcode >> 7) & 0x1F;
	std::uint32_t imm = (imm_11_5 << 5) | imm_4_0;

	std::int32_t signed_imm = (imm & (1 << 11)) ? (imm | ~((1 << 12) - 1)) : imm;

	std::uint8_t rs1 = (opcode >> 15) & 0x1F;
	std::uint8_t rs2 = (opcode >> 20) & 0x1F;

	uint32_t effective_address = core->registers[rs1] + signed_imm;

	core->bus.write32(effective_address, core->registers[rs2]);
}

// AMO
void RV32IInterpreter::rv32i_amoor_w(std::uint32_t opcode) {
    std::uint8_t rd = (opcode >> 7) & 0x1F;
    std::uint8_t rs1 = (opcode >> 15) & 0x1F;
    std::uint8_t rs2 = (opcode >> 20) & 0x1F;

    std::uint32_t address = core->registers[rs1];
    std::uint32_t value = core->bus.read32(address);
    std::uint32_t result = value | core->registers[rs2];
    core->bus.write32(address, result);

    core->registers[rd] = value;
}

void RV32IInterpreter::rv32i_amoadd_w(std::uint32_t opcode) {
    std::uint8_t rd = (opcode >> 7) & 0x1F;
    std::uint8_t rs1 = (opcode >> 15) & 0x1F;
    std::uint8_t rs2 = (opcode >> 20) & 0x1F;

    std::uint32_t address = core->registers[rs1];
    std::uint32_t value = core->bus.read32(address);
    std::uint32_t result = value + core->registers[rs2];
    core->bus.write32(address, result);

    core->registers[rd] = value;
}