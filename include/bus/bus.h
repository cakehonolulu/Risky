#pragma once

#include <cstdint>
#include <string>
#include <fstream>
#include <iostream>

class Bus {

public:

	Bus();
	~Bus();

	std::uint8_t* main_memory; // Pointer for main memory (16MB)

	void load_binary(const std::string& binary_path);

	std::uint32_t read32(std::uint32_t address);
};