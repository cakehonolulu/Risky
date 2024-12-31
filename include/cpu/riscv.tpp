#include <iostream>
#include <iomanip>

#include <log/log.hh>
#include <risky.h>
#include <cstring>

#if __has_include(<format>)
#include <format>
using std::format;
#else
#include <fmt/format.h>
using fmt::format;
#endif

template <std::uint8_t xlen, bool is_embedded>
RISCV<xlen, is_embedded>::RISCV(const std::vector<std::string>& extensions)
		: extensions(extensions), pc(0x80000000) {
	std::memset(registers, 0, sizeof(registers));
	std::memset(csrs, 0, sizeof(csrs));
	has_a = false;
	has_m = false;
	has_zicsr = false;
    has_zifencei = false;
    has_compressed = false;

	Logger::set_subsystem("CORE");

	for (const std::string& ext : extensions) {
		if (ext == "A") {
			has_a = true;
		} else if (ext == "M") {
			has_m = true;
		} else if (ext == "Zicsr") {
			has_zicsr = true;
		} else if (ext == "Zifencei") {
			has_zifencei = true;
        } else if (ext == "C") {
            has_compressed = true;
        }
	}
}

template <std::uint8_t xlen, bool is_embedded>
RISCV<xlen, is_embedded>::~RISCV() {}

template <std::uint8_t xlen, bool is_embedded>
void RISCV<xlen, is_embedded>::run_() {
	running = true;
}

template <std::uint8_t xlen, bool is_embedded>
void RISCV<xlen, is_embedded>::step_() {
	if (running) {
		step();
	}
}

template <std::uint8_t xlen, bool is_embedded>
void RISCV<xlen, is_embedded>::stop() {
	running = false;
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

template <std::uint8_t XLEN, bool is_embedded>
void RISCV<XLEN, is_embedded>::set_run_func(std::function<void()> run_func) {
	run = std::move(run_func);
}

template <std::uint8_t xlen, bool is_embedded>
std::uint32_t RISCV<xlen, is_embedded>::fetch_opcode() {
	return bus.read32(pc);
}

template <std::uint8_t xlen, bool is_embedded>
std::uint32_t RISCV<xlen, is_embedded>::fetch_opcode(addr_t pc) {
	static_assert(xlen == 32 || xlen == 64 || xlen == 128, "Unsupported XLEN");
    
	return bus.read32(pc);
}
