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

std::string Disassembler::Disassemble(uint32_t opcode) {
    // Check if RV16 or RV32
    if ((opcode & 0x3) != 0x3) {
        return DecodeRV16(static_cast<std::uint16_t>(opcode));
    } else {
        return DecodeRV32(opcode);
    }
}

std::string Disassembler::DecodeRV32(uint32_t opcode) {
    uint32_t funct3 = (opcode >> 12) & 0x7;
    uint32_t funct7 = (opcode >> 25) & 0x7F;
    uint32_t rd = (opcode >> 7) & 0x1F;
    uint32_t rs1 = (opcode >> 15) & 0x1F;
    uint32_t rs2 = (opcode >> 20) & 0x1F;
    uint32_t imm = (opcode >> 20);
    uint32_t csr = (opcode >> 20) & 0xFFF;

    switch (opcode & 0x7F) {
        case LUI: // LUI
            return "lui " + std::string(cpu_register_names[rd]) + ", " + std::to_string(imm);

        case MISCMEM:
            switch (funct3) {
                case 0b001:
                    return "fence.i";
            }

        case AUIPC: // AUIPC
            return "auipc " + std::string(cpu_register_names[rd]) + ", " + std::to_string(imm);
        case JAL: // JAL
            return "jal " + std::string(cpu_register_names[rd]) + ", " + std::to_string(imm);
        case JALR: // JALR
            return "jalr " + std::string(cpu_register_names[rd]) + ", " + std::string(cpu_register_names[rs1]) + ", " + std::to_string(imm);
        case SYSTEM: // SYSTEM
            switch (funct3) {
                case 0x1:
                    return "csrrw " + std::string(cpu_register_names[rd]) + ", " + get_csr_name(csr) + ", " + std::string(cpu_register_names[rs1]);
                case 0x2:
                    return "csrrs " + std::string(cpu_register_names[rd]) + ", " + get_csr_name(csr) + ", " + std::string(cpu_register_names[rs1]);
                case 0x3:
                    return "csrrc " + std::string(cpu_register_names[rd]) + ", " + get_csr_name(csr) + ", " + std::string(cpu_register_names[rs1]);
                case 0x5:
                    return "csrrsi " + std::string(cpu_register_names[rd]) + ", " + get_csr_name(csr) + ", " + std::to_string(rs1); // rs1 is an immediate value here
            }
            break;
        case OPIMM: // OP-IMM
            if (funct3 == 0x0)
                return "addi " + std::string(cpu_register_names[rd]) + ", " + std::string(cpu_register_names[rs1]) + ", " + std::to_string(imm);
            break;
        case BRANCH: // BRANCH
            switch (funct3) {
                case 0x4:
                    return "blt " + std::string(cpu_register_names[rs1]) + ", " + std::string(cpu_register_names[rs2]) + ", " + std::to_string(imm);
                case 0x5:
                    return "bge " + std::string(cpu_register_names[rs1]) + ", " + std::string(cpu_register_names[rs2]) + ", " + std::to_string(imm);
            }
            break;
        case STORE: // STORE
            if (funct3 == 0x2)
                return "sw " + std::string(cpu_register_names[rs2]) + ", " + std::to_string(imm) + "(" + std::string(cpu_register_names[rs1]) + ")";
            break;
    }
    return "UNKNOWN_OPCODE";
}

std::string Disassembler::DecodeRV16(uint16_t opcode) {
    uint8_t op = opcode & 0x3;          // bits [1:0]
    uint8_t funct3 = (opcode >> 13) & 0x7; // bits [15:13]

    if (op == 0x1 && funct3 == 0) {
        uint8_t rd = (opcode >> 7) & 0x1F; // bits [11:7]
        uint8_t imm5 = (opcode >> 2) & 0x1F; // bits [6:2]
        bool imm_high_bit = opcode & 0x1000; // bit [12]

        // Calculate the immediate value
        int32_t imm = imm_high_bit ? 0xFFFFFFC0 : 0; // imm[31:6]
        imm |= (imm_high_bit ? 0x20 : 0) | imm5; // imm[5:0]

        if (rd == 0 && imm == 0) {
            return "c.nop"; // No operation
        } else if (rd != 0) {
            return "c.addi x" + std::to_string(rd) + ", " + std::to_string(imm);
        }
    }

    return "UNKNOWN_RV16_OPCODE";
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