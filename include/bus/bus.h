#pragma once

#include <cstdint>
#include <string>
#include <fstream>
#include <iostream>
#include "risky.h"
#include <log/log.hh>

#define UART                        0x10000000
#define UART_THR                    (UART + 0x00)
#define UART_LSR                    (UART + 0x05)

class Bus {

public:

	Bus();
	~Bus();

	std::uint8_t* main_memory;  // Pointer for main memory
	std::size_t main_memory_size;    // Size of main memory in bytes

	void load_binary(const std::string& binary_path);

	std::uint8_t read8(std::uint32_t address);
	std::uint32_t read32(std::uint32_t address);
	void write8(uint32_t address, uint8_t value);
	void write32(uint32_t address, uint32_t value);
};