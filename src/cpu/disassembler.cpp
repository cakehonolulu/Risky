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
	SetDisassembleFunction(JAL, [](uint32_t opcode) {
		// rd -> [11:7]
		std::uint8_t rd = (opcode >> 7) & 0x1F;

		int32_t imm =
				static_cast<int32_t> (((opcode >> 31) & 0x1) << 20 |
				((opcode >> 21) & 0x3FF) << 1 |
				((opcode >> 20) & 0x1)) << 11 >> 11;

		return format("jal x{}, {}", rd, imm);
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