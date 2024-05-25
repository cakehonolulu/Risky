#include <cpu/disassembler.h>
#include <cpu/registers.h>
#include <cpu/riscv.h>

#if __has_include(<format>)
#include <format>
using std::format;
#else
#include <fmt/format.h>
using format;
#endif

std::string Disassembler::Disassemble(uint32_t opcode, const std::vector<std::string> *regnames) {
    // Check if RV16 or RV32
    if ((opcode & 0x3) != 0x3) {
        return DecodeRV16(static_cast<std::uint16_t>(opcode), regnames);
    } else {
        return DecodeRV32(opcode, regnames);
    }
}

std::string Disassembler::DecodeRV16(uint16_t opcode, const std::vector<std::string> *regnames) {
	uint16_t funct3 = (opcode >> 13) & 0x7;
	uint16_t imm = (opcode >> 2) & 0x1F;
	uint16_t rd_rs1 = (opcode >> 7) & 0x1F;
	uint16_t imm6 = (opcode >> 12) & 0x1;
	imm = (imm6 << 5) | imm;

	if (funct3 == 0b000) {
		if (rd_rs1 == 0 && imm == 0) {
			return "c.nop";
		} else {
			return "c.addi " + std::string(regnames->at(rd_rs1)) + ", " + std::string(regnames->at(rd_rs1)) + ", " + std::to_string(static_cast<int8_t>(imm));
		}
	}

	return "UNKNOWN_RV16_OPCODE";
}

std::string Disassembler::DecodeRV32(uint32_t opcode, const std::vector<std::string> *regnames) {
    uint32_t funct3 = (opcode >> 12) & 0x7;
    uint32_t funct7 = (opcode >> 25) & 0x7F;
    uint32_t rd = (opcode >> 7) & 0x1F;
    uint32_t rs1 = (opcode >> 15) & 0x1F;
    uint32_t rs2 = (opcode >> 20) & 0x1F;
	int32_t imm = static_cast<int32_t>(opcode) >> 20;
    uint32_t csr = (opcode >> 20) & 0xFFF;

    switch (opcode & 0x7F) {
	    case LUI: // LUI
		    return "lui " + std::string(regnames->at(rd)) + ", " + std::to_string(imm);

		case LOAD:
			switch (funct3) {
				case 0x2: // LW
					return "lw " + std::string(regnames->at(rd)) + ", " + std::to_string(imm) + "(" +
					       std::string(regnames->at(rs1)) + ")";
				case 0x4: // LBU
					return "lbu " + std::string(regnames->at(rd)) + ", " + std::to_string(imm) + "(" +
					       std::string(regnames->at(rs1)) + ")";
			}
		    break;

	    case MISCMEM:
		    switch (funct3) {
			    case 0b001:
				    return "fence.i";
		    }

	    case AUIPC: // AUIPC
		    imm = opcode & 0xFFFFF000;
		    return "auipc " + std::string(regnames->at(rd)) + ", " + std::to_string(static_cast<int32_t>(imm) >> 12);
	    case JAL: // JAL
		    return "jal " + std::string(regnames->at(rd)) + ", " + std::to_string(imm);
	    case JALR: // JALR
		    return "jalr " + std::string(regnames->at(rd)) + ", " + std::string(regnames->at(rs1)) + ", " +
		           std::to_string(imm);
	    case SYSTEM: // SYSTEM
		    switch (funct3) {
			    case 0x1:
				    return "csrrw " + std::string(regnames->at(rd)) + ", " + get_csr_name(csr) + ", " +
				           std::string(regnames->at(rs1));
			    case 0x2:
				    return "csrrs " + std::string(regnames->at(rd)) + ", " + get_csr_name(csr) + ", " +
				           std::string(regnames->at(rs1));
			    case 0x3:
				    return "csrrc " + std::string(regnames->at(rd)) + ", " + get_csr_name(csr) + ", " +
				           std::string(regnames->at(rs1));
			    case 0x5:
				    return "csrrsi " + std::string(regnames->at(rd)) + ", " + get_csr_name(csr) + ", " +
				           std::to_string(rs1); // rs1 is an immediate value here
			    case 0x7:
				    return "csrrwi " + std::string(regnames->at(rd)) + ", " + get_csr_name(csr) + ", " +
				           std::to_string(rs1); // rs1 is an immediate value here
		    }
		    break;
	    case OPIMM: // OP-IMM
		    switch (funct3) {
			    case 0x0: // ADDI
				    return "addi " + std::string(regnames->at(rd)) + ", " + std::string(regnames->at(rs1)) + ", " +
				           std::to_string(imm);
			    case 0x7: // ANDI
				    return "andi " + std::string(regnames->at(rd)) + ", " + std::string(regnames->at(rs1)) + ", " +
				           std::to_string(imm);
		    }
		    break;
	    case BRANCH: // BRANCH
		    switch (funct3) {
			    case 0x0: // BEQ
				    return "beq " + std::string(regnames->at(rs1)) + ", " + std::string(regnames->at(rs2)) + ", " +
				           format_branch_imm(opcode);
			    case 0x1: // BNE
				    return "bne " + std::string(regnames->at(rs1)) + ", " + std::string(regnames->at(rs2)) + ", " +
				           format_branch_imm(opcode);
			    case 0x4: // BLT
				    return "blt " + std::string(regnames->at(rs1)) + ", " + std::string(regnames->at(rs2)) + ", " +
				           format_branch_imm(opcode);
			    case 0x5: // BGE
				    return "bge " + std::string(regnames->at(rs1)) + ", " + std::string(regnames->at(rs2)) + ", " +
				           format_branch_imm(opcode);
			    case 0x6: // BLTU
				    return "bltu " + std::string(regnames->at(rs1)) + ", " + std::string(regnames->at(rs2)) + ", " +
				           format_branch_imm(opcode);
			    case 0x7: // BGEU
				    return "bgeu " + std::string(regnames->at(rs1)) + ", " + std::string(regnames->at(rs2)) + ", " +
				           format_branch_imm(opcode);
		    }
		    break;
	    case STORE: // STORE
			switch (funct3)
			{
				case 0x00:
				{
					int32_t imm_11_5 = (opcode >> 25) & 0x7F;
					int32_t imm_4_0 = (opcode >> 7) & 0x1F;
					int32_t imm_store = (imm_11_5 << 5) | imm_4_0;
					imm_store = (imm_store << 20) >> 20;
					return "sb " + std::string(regnames->at(rs2)) + ", " + std::to_string(imm_store) + "(" +
					       std::string(regnames->at(rs1)) + ")";
				}

				case 0x02:
				{
					int32_t imm_11_5 = (opcode >> 25) & 0x7F;
					int32_t imm_4_0 = (opcode >> 7) & 0x1F;
					int32_t imm_store = (imm_11_5 << 5) | imm_4_0;
					imm_store = (imm_store << 20) >> 20;
					return "sw " + std::string(regnames->at(rs2)) + ", " + std::to_string(imm_store) + "(" +
					       std::string(regnames->at(rs1)) + ")";
				}
			}
			break;
    }
    return "UNKNOWN_OPCODE";
}

std::string Disassembler::get_csr_name(uint16_t csr) {
    switch (csr) {
        case 0x105: return "stvec";
        case 0x300: return "mstatus";
        case 0x304: return "mie";
        case 0x305: return "mtvec";
        case 0x340: return "mscratch";
        case 0x344: return "mip";
        case 0x3A0: return "pmpcfg0";
        case 0x3B0: return "pmpaddr0";
        case 0xF14: return "mhartid";
        default:
            std::cerr << "[RIKSY] disassembler: get_csr_name() -> Unknown CSR: 0x" << std::hex << csr << std::dec << std::endl;
            return "UNKNOWN_CSR";
    }
}

std::string Disassembler::format_branch_imm(uint32_t opcode) {
	std::int32_t imm_12 = (opcode >> 31) & 0x1;
	std::int32_t imm_10_5 = (opcode >> 25) & 0x3F;
	std::int32_t imm_4_1 = (opcode >> 8) & 0xF;
	std::int32_t imm_11 = (opcode >> 7) & 0x1;

	std::int32_t imm = (imm_12 << 12) | (imm_11 << 11) | (imm_10_5 << 5) | (imm_4_1 << 1);
	imm = (imm << 19) >> 19;

	std::ostringstream oss;
	oss << imm;
	return oss.str();
}
