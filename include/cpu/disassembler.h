#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <unordered_map>

class Disassembler {
public:
	using DisassembleFunction = std::function<std::string(uint32_t)>;

	Disassembler();

	void SetDisassembleFunction(uint8_t opcode, DisassembleFunction func);

	std::string Disassemble(uint32_t opcode);

	std::string get_csr_name(std::uint16_t csr);

private:
	std::unordered_map<uint8_t, DisassembleFunction> opcodeFunctions;
};
