#include <bus/bus.h>
#include <log/log.h>

Bus::Bus()
{
	main_memory = new std::uint8_t[16 * 1024 * 1024];
}

Bus::~Bus() {
	delete[] main_memory;
}

#include <iostream>
#include <iomanip>

void Bus::load_binary(const std::string& binary_path)
{
	std::ifstream binary_file(binary_path, std::ios::binary);

	if (!binary_file.is_open()) {
		Logger::Instance().Error("[BUS] Failed to open the binary file: " + binary_path);
		Risky::exit();
	}
	else
	{
		Logger::Instance().Log("[BUS] Binary file opened successfully...!");
	}

	const uint32_t binary_base_addr = 0x00000000;
	const uint32_t binary_end_addr = binary_base_addr + 0x0003FFFF;
	uint32_t offset = binary_base_addr;

	while (!binary_file.eof() && offset <= binary_end_addr) {
		char byte;
		binary_file.read(&byte, 1);
		main_memory[offset] = static_cast<uint8_t>(byte);
		offset++;
	}

	binary_file.close();
}

std::uint32_t Bus::read32(std::uint32_t address)
{
	if (address >= 0x80000000 && address < (0x80000000 + (16 * 1024 * 1024)))
	{
		return  (main_memory[(address - 0x80000000) + 3] << 24) |
				(main_memory[(address - 0x80000000) + 2] << 16) |
				(main_memory[(address - 0x80000000) + 1] << 8) |
				main_memory[(address - 0x80000000) + 0];
	}

	return 0x00000000;
}
