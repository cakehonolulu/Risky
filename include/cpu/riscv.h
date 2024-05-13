#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <unordered_map>

#include <bus/bus.h>

#define EMBEDDED true

#define JAL 0b1101111

template <std::uint8_t xlen, bool is_embedded = false>
class RISCV {
public:
	RISCV(const std::vector<std::string>& extensions);
	~RISCV();

	Bus bus;

	std::function<void()> step;
	void run();
	void reset();

	void set_step_func(std::function<void()> step_func);

	std::uint32_t fetch_opcode();
	void unknown_opcode(std::uint32_t opcode);

	typename std::conditional<(xlen == 32 && xlen != 16), std::uint32_t,
			typename std::conditional<(xlen == 64), std::uint64_t,
							std::uint32_t // Default to 32-bit if unknown xlen
			>::type
	>::type registers[is_embedded ? 16 : 32];

	typename std::conditional<(xlen == 32), std::uint32_t,
			typename std::conditional<(xlen == 64), std::uint64_t,
							std::uint32_t // Default to 32-bit if unknown xlen
			>::type
	>::type pc;

private:
	std::vector<std::string> extensions;
};

#include <cpu/riscv.tpp>
