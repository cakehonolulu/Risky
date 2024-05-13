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

Disassembler::Disassembler() {

	SetDisassembleFunction(JAL, [this](uint32_t opcode) {
		// rd -> [11:7]
		std::uint8_t rd = (opcode >> 7) & 0x1F;

		int32_t imm =
				static_cast<int32_t> (((opcode >> 31) & 0x1) << 20 |
				                      ((opcode >> 21) & 0x3FF) << 1 |
				                      ((opcode >> 20) & 0x1)) << 11 >> 11;

		return format("jal {}, {}", cpu_register_names[rd], imm);
	});

	SetDisassembleFunction(JALR, [this](uint32_t opcode) {
		std::uint8_t rd = (opcode >> 7) & 0x1F;
		std::uint8_t rs1 = (opcode >> 15) & 0x1F;
		std::int32_t imm = static_cast<std::int32_t>((opcode >> 20) & 0xFFF);

		std::string imm_str = imm >= 0 ? format("{:#x}", imm) : format("-{:#x}", -imm);

		return format("jalr {}, {}({})", cpu_register_names[rd], imm_str, cpu_register_names[rs1]);
	});

	SetDisassembleFunction(MISCMEM, [this](uint32_t opcode) {
		std::uint8_t funct3 = (opcode >> 12) & 0x7;

		std::string instruction;

		switch (funct3) {
			case 0b001:
				instruction = "fence.i";
				break;
			default:
				instruction = "UNKNOWN_MISCMEM_OPCODE";
				break;
		}

		if (instruction == "UNKNOWN_MISCMEM_OPCODE") {
			std::string unknown_miscmem = "UNKNOWN_MISCMEM_OPCODE";
			return unknown_miscmem;
		}

		return instruction;
	});

	SetDisassembleFunction(OPIMM, [this](uint32_t opcode) {
		std::uint8_t rd = (opcode >> 7) & 0x1F;
		std::uint8_t rs1 = (opcode >> 15) & 0x1F;
		std::uint8_t funct3 = (opcode >> 12) & 0x7;
		int32_t imm = static_cast<int32_t>(static_cast<int32_t>(opcode) >> 20);

		std::string instruction;

		switch (funct3) {
			case 0b000:
				instruction = "addi";
				break;
			case 0b010:
				instruction = "slti";
				break;
			case 0b011:
				instruction = "sltiu";
				break;
			default:
				instruction = "UNKNOWN_IMMEDIATE_OPCODE";
				break;
		}

		if (instruction == "UNKNOWN_IMMEDIATE_OPCODE") {
			std::string unknown_immediate = "UNKNOWN_IMMEDIATE_OPCODE";
			return unknown_immediate;
		}

		return format("{} {}, {}, {}", instruction, cpu_register_names[rd], cpu_register_names[rs1], imm);
	});

	SetDisassembleFunction(SYSTEM, [this](uint32_t opcode) {
		std::uint8_t rd = (opcode >> 7) & 0x1F;
		std::uint8_t rs1 = (opcode >> 15) & 0x1F;
		std::uint16_t csr = (opcode >> 20) & 0xFFF;
		std::uint8_t funct3 = (opcode >> 12) & 0x7;

		std::string instruction;

		switch (funct3) {
			case 0b001:
				instruction = "csrrw";
				break;
			case 0b101:
				instruction = "csrrw";
				break;
			default:
				instruction = "UNKNOWN_ZICSR_OPCODE";
				break;
		}

		std::string csr_name = get_csr_name(csr);

		if (instruction == "UNKNOWN_ZICSR_OPCODE")
		{
			std::string unknown_zicsr = "UNKNOWN_ZICSR_OPCODE";
			return unknown_zicsr;
		}

		return format("{} {}, {}, {}", instruction, cpu_register_names[rd], csr_name, cpu_register_names[rs1]);
	});
}

void Disassembler::SetDisassembleFunction(uint8_t opcode, DisassembleFunction func) {
	opcodeFunctions[opcode] = func;
}

std::string Disassembler::Disassemble(uint32_t opcode) {
	uint8_t opcodeKey = opcode & 0x7F;
	auto it = opcodeFunctions.find(opcodeKey);

	if (it != opcodeFunctions.end()) {
		return it->second(opcode);
	} else {
		return format("UNKNOWN_OPCODE 0x{:08X}", opcode);
	}
}

std::string Disassembler::get_csr_name(std::uint16_t csr) {
	switch (csr) {
		case 0x105: return "stvec";
		case 0x300: return "mstatus";
		case 0x305: return "mtvec";
		case 0x340: return "mscratch";
		case 0x344: return "mip";
		case 0x3A0: return "pmpcfg0";
		case 0x3B0: return "pmpaddr0";
		case 0xF14: return "mhartid";
		default:
			Logger::Instance().Log(format("[RIKSY] disassembler: get_csr_name() -> Unknown CSR: 0x{:04X}", csr));
			return "UNKNOWN_CSR";
	}
}
