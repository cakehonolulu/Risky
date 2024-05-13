#include <iostream>
#include <iomanip>

#include <log/log.h>
#include <risky.h>
#include <cstring>

template <std::uint8_t xlen, bool is_embedded>
RISCV<xlen, is_embedded>::RISCV(const std::vector<std::string>& extensions)
		: extensions(extensions), pc(0x80000000) {
	std::memset(registers, 0, sizeof(registers));
}

template <std::uint8_t xlen, bool is_embedded>
RISCV<xlen, is_embedded>::~RISCV() {}

template <std::uint8_t xlen, bool is_embedded>
void RISCV<xlen, is_embedded>::run() {
	while (true) {
		step();
	}
}

template <std::uint8_t xlen, bool is_embedded>
void RISCV<xlen, is_embedded>::reset() {
    std::memset(registers, 0, sizeof(registers));
    pc = 0x80000000;
}

template <std::uint8_t XLEN, bool is_embedded>
void RISCV<XLEN, is_embedded>::set_step_func(std::function<void()> step_func) {
	step = std::move(step_func);
}

template <std::uint8_t xlen, bool is_embedded>
std::uint32_t RISCV<xlen, is_embedded>::fetch_opcode() {
	return bus.read32(pc);
}

template <std::uint8_t xlen, bool is_embedded>
void RISCV<xlen, is_embedded>::unknown_opcode(std::uint32_t opcode) {

	std::ostringstream logMessage;
	logMessage << "[RISKY] Unimplemented opcode: 0x" << format("{:08X}", opcode);

	Logger::Instance().Error(logMessage.str());

	Risky::exit();
}
