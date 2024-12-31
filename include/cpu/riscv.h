#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <unordered_map>

#include <bus/bus.h>
#include <log/log.hh>
#include "risky.h"

#define EMBEDDED true

#define LOAD    0b0000011
#define MISCMEM 0b0001111
#define OPIMM   0b0010011
#define STORE   0b0100011
#define AMO     0b0101111
#define OP      0b0110011
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

	using addr_t = std::conditional_t<xlen == 32, uint32_t, std::conditional_t<xlen == 64, uint64_t, __uint128_t>>;

	Bus bus;

	std::function<void()> step;
	std::function<void()> run;
	void run_();
	void step_();
	void stop();
	bool isRunning() const { return running; }
	void reset();

	void set_step_func(std::function<void()> step_func);
	void set_run_func(std::function<void()> run_func);

	std::uint32_t fetch_opcode();
	std::uint32_t fetch_opcode(addr_t pc);

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
		}
		
		Logger::error("csr_read: Invalid CSR register read: " + std::to_string(csr));
		return Risky::exit(1, Risky::Subsystem::Core);
	}

	void csr_write(std::uint16_t csr, typename std::conditional<(xlen == 32), std::uint32_t,
			typename std::conditional<(xlen == 64), std::uint64_t,
					std::uint32_t // Default to 32-bit if unknown xlen
			>::type
	>::type value) {
		if (csr < 4096) {
			csrs[csr] = value;
		} else {
			Logger::error("csr_write: Invalid CSR register write: " + std::to_string(csr));
			Risky::exit(1, Risky::Subsystem::Core);
		}
	}

	bool has_a;
	bool has_m;
	bool has_zicsr;
	bool has_zifencei;
    bool has_compressed;
	bool running = false;

private:
	std::vector<std::string> extensions;
};

#include <cpu/riscv.tpp>
