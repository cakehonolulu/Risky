#include <cpu/disassembler.h>
#include <cpu/registers.h>

#if __has_include(<format>)
#include <format>
using std::format;
#else
#include <fmt/format.h>
using format;
#endif

Disassembler::Disassembler() {
	SetDisassembleFunction(0b1101111, [](uint32_t opcode) {
		int rd = (opcode >> 7) & 0x1F;
		int imm = ((opcode >> 12) & 0xFF) | ((opcode >> 20) & 0x7FE) | ((opcode >> 11) & 0x1000) | ((opcode & 0x80000000) ? 0xFFF00000 : 0);
		return format("jal x{}, {}", rd, imm);
	});

	SetDisassembleFunction(0b1100111, [](uint32_t opcode) {
		int rd = (opcode >> 7) & 0x1F;
		int rs1 = (opcode >> 15) & 0x1F;
		int imm = static_cast<int>(static_cast<int32_t>(opcode) >> 20);
		return format("jalr x{}, x{}, {}", rd, rs1, imm);
	});

	SetDisassembleFunction(0b1100011, [](uint32_t opcode) {
		int rs1 = (opcode >> 15) & 0x1F;
		int rs2 = (opcode >> 20) & 0x1F;
		int imm = ((opcode >> 7) & 0x1E) | ((opcode >> 20) & 0x7E0) | ((opcode & 0x80000000) ? 0xFFFFF000 : 0);
		int funct3 = (opcode >> 12) & 0x7;
		std::string mnemonic;
		switch (funct3) {
			case 0b000: mnemonic = "beq"; break;
			case 0b001: mnemonic = "bne"; break;
			case 0b100: mnemonic = "blt"; break;
			case 0b101: mnemonic = "bge"; break;
			case 0b110: mnemonic = "bltu"; break;
			case 0b111: mnemonic = "bgeu"; break;
			default: mnemonic = "UNKNOWN_BRANCH";
		}
		return format("{} x{}, x{}, {}", mnemonic, rs1, rs2, imm);
	});
}

void Disassembler::SetDisassembleFunction(uint8_t opcode, DisassembleFunction func) {
	opcodeFunctions[opcode] = func;
}

std::string Disassembler::Disassemble(uint32_t opcode) {
	uint8_t opcodeKey = opcode & 0x7F; // Extract opcode key from the opcode
	auto it = opcodeFunctions.find(opcodeKey);

	if (it != opcodeFunctions.end()) {
		return it->second(opcode);
	} else {
		return format("UNKNOWN_OPCODE 0x{:08X}", opcode);
	}
}