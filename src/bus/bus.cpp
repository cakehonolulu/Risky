#include <bus/bus.h>
#include <log/log.hh>
#include <iostream>
#include <iomanip>

Bus::Bus()
{
	Logger::set_subsystem("BUS");
	main_memory_size = 16 * 1024 * 1024;
	main_memory = new std::uint8_t[main_memory_size];
}

Bus::~Bus() {
	delete[] main_memory;
}

void Bus::load_binary(const std::string& binary_path)
{
	std::ifstream binary_file(binary_path, std::ios::binary);

	if (!binary_file.is_open()) {
		Logger::error("Failed to open the binary file: " + binary_path);
		Risky::exit(1, Risky::Subsystem::Bus);
	}
	else
	{
		Logger::info("Binary file opened successfully...!");
	}

	const uint32_t binary_base_addr = 0x00000000;
	uint32_t offset = binary_base_addr;

	// Seek to the end to get the file size
	binary_file.seekg(0, std::ios::end);
	std::streampos file_size = binary_file.tellg();
	binary_file.seekg(0, std::ios::beg);

	// Allocate buffer to read the entire file
	std::vector<char> buffer(static_cast<size_t>(file_size));

	// Read the entire file into the buffer
	if (binary_file.read(buffer.data(), file_size)) {
		for (size_t i = 0; i < static_cast<size_t>(file_size); ++i) {
			main_memory[offset] = static_cast<uint8_t>(buffer[i]);
			offset++;
		}
	} else {
		Logger::error("Failed to read the binary file: " + binary_path);
		Risky::exit(1, Risky::Subsystem::Bus);
	}

	binary_file.close();
}

std::uint8_t Bus::read8(std::uint32_t address)
{
	if (address == UART_THR)
	{
		//Logger::Instance().Log("[MEM] read8: Read from UART_THR");
		return 0x00;
	}
	else if (address == UART_LSR)
	{
		//Logger::Instance().Log("[MEM] read8: Read from UART_LSR");
		return 0x60;
	}
	else if (address >= 0x80000000 && address < (0x80000000 + main_memory_size))
	{
		std::size_t offset = address - 0x80000000;

		return main_memory[offset];
	}
	else if (address >= 0x7F000000 && address < (0x80000000 + main_memory_size))
	{
		// TODO: Minor hack for debugger not to exit
		return 0x00;
	}

	std::stringstream errorMessage;
	errorMessage << "read8: Unhandled memory address: 0x"
	             << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << address;

	Logger::error(errorMessage.str());
	Risky::exit(1, Risky::Subsystem::Bus);
	return 0x00;
}

std::uint32_t Bus::read32(std::uint32_t address)
{
	if (address >= 0x80000000 && address < (0x80000000 + main_memory_size))
	{
		std::size_t offset = address - 0x80000000;
		
		return  (main_memory[offset + 3] << 24) |
				(main_memory[offset + 2] << 16) |
				(main_memory[offset + 1] << 8) |
				main_memory[offset + 0];
	}
    else if (address >= 0x7F000000 && address < (0x80000000 + main_memory_size))
    {
        // TODO: Minor hack for debugger not to exit
        return 0x00000000;
    }

	std::stringstream errorMessage;
	errorMessage << "read32: Unhandled memory address: 0x"
	             << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << address;

	Logger::error(errorMessage.str());
	Risky::exit(1, Risky::Subsystem::Bus);
	return 0x00000000;
}

void Bus::write8(std::uint32_t address, std::uint8_t value)
{
	if (address == UART_THR)
	{
		static std::string uart_buffer;
		if (value == '\n' || uart_buffer.size() >= 256) {
			Logger::uart(uart_buffer);
			uart_buffer.clear();
		} else {
			uart_buffer += static_cast<char>(value);
		}
	}
	else if (address >= 0x80000000 && address < (0x80000000 + main_memory_size))
	{
		std::size_t offset = address - 0x80000000;

		main_memory[offset] = static_cast<std::uint8_t>(value);
	}
	else
	{
		std::stringstream errorMessage;
		errorMessage << "write8: Unhandled memory address: 0x"
		             << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << address;

		Logger::error(errorMessage.str());
		Risky::exit(1, Risky::Subsystem::Bus);
	}
}

void Bus::write32(std::uint32_t address, std::uint32_t value)
{
	if (address >= 0x80000000 && address < (0x80000000 + main_memory_size))
	{
		std::size_t offset = address - 0x80000000;

		main_memory[offset + 0] = static_cast<std::uint8_t>(value & 0xFF);
		main_memory[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
		main_memory[offset + 2] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
		main_memory[offset + 3] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
	}
	else
	{
		std::stringstream errorMessage;
		errorMessage << "write32: Unhandled memory address: 0x"
		             << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << address;

		Logger::error(errorMessage.str());
		Risky::exit(1, Risky::Subsystem::Bus);
	}
}
