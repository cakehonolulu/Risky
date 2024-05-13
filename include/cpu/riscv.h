#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <unordered_map>

#include <bus/bus.h>
#include <log/log.h>

#define EMBEDDED true

#define MISCMEM 0b0001111
#define OPIMM   0b0010011
#define BRANCH  0b1100011
#define SYSTEM  0b1110011

#define AUIPC   0b0010111
#define LUI     0b0110111
#define JAL     0b1101111
#define JALR    0b1100111

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

	typename std::conditional<(xlen == 32), std::uint32_t,
			typename std::conditional<(xlen == 64), std::uint64_t,
					std::uint32_t // Default to 32-bit if unknown xlen
			>::type
	>::type csrs[4096];

	typename std::conditional<(xlen == 32), std::uint32_t,
			typename std::conditional<(xlen == 64), std::uint64_t,
					std::uint32_t // Default to 32-bit if unknown xlen
			>::type
	>::type csr_read(std::uint16_t csr) {
		if (csr < 4096) {
			return csrs[csr];
		} else {
			Logger::Instance().Error("[RISCV] csr_read: Invalid CSR register read: " + std::to_string(csr));
			Risky::exit();
		}
	}

	void csr_write(std::uint16_t csr, typename std::conditional<(xlen == 32), std::uint32_t,
			typename std::conditional<(xlen == 64), std::uint64_t,
					std::uint32_t // Default to 32-bit if unknown xlen
			>::type
	>::type value) {
		if (csr < 4096) {
			csrs[csr] = value;
		} else {
			Logger::Instance().Error("[RISCV] csr_write: Invalid CSR register write: " + std::to_string(csr));
			Risky::exit();
		}
	}

	bool has_a;
	bool has_m;
	bool has_zicsr;
	bool has_zifence;

private:
	std::vector<std::string> extensions;
};

#include <cpu/riscv.tpp>
