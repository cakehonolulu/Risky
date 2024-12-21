#include <cpu/core/core.h>
#include <log/log.hh>
#include <risky.h>
#include <fstream>
#include <vector>
#include <cstring>

template <typename T>
void Core::load_binary(const std::string& filePathName) {
    if (is_embedded) {
        auto riscvInstance = std::any_cast<RISCV<sizeof(T) * 8, true>*>(&riscv);
        if (riscvInstance) {
            (*riscvInstance)->bus.load_binary(filePathName);
        } else {
            Logger::error("Incompatible RISCV instance");
            Risky::exit(1, Risky::Subsystem::Core);
        }
    } else {
        auto riscvInstance = std::any_cast<RISCV<sizeof(T) * 8, false>*>(&riscv);
        if (riscvInstance) {
            (*riscvInstance)->bus.load_binary(filePathName);
        } else {
            Logger::error("Incompatible RISCV instance");
            Risky::exit(1, Risky::Subsystem::Core);
        }
    }
}

template <typename T>
bool Core::load_elf(const std::string& filename) {
    std::ifstream elf_file(filename, std::ios::binary);
    if (!elf_file) {
        Logger::error("load_elf: Could not open ELF file " + filename);
        return false;
    }

    if constexpr (sizeof(T) == 4) {
        // Read ELF header
        Elf32_Ehdr header;
        elf_file.read(reinterpret_cast<char*>(&header), sizeof(header));

        // Verify ELF magic number
        if (memcmp(header.e_ident, ELFMAG, SELFMAG) != 0) {
            Logger::error("load_elf: " + filename + " is not a valid ELF file");
            return false;
        }

        // Load program headers
        elf_file.seekg(header.e_phoff, std::ios::beg);
        std::vector<Elf32_Phdr> program_headers(header.e_phnum);
        elf_file.read(reinterpret_cast<char*>(program_headers.data()), header.e_phnum * sizeof(Elf32_Phdr));

        // Load sections into memory
        for (const auto& phdr : program_headers) {
            if (phdr.p_type != PT_LOAD) {
                continue;
            }

            // Read the segment from the file
            std::vector<char> segment_data(phdr.p_filesz);
            elf_file.seekg(phdr.p_offset, std::ios::beg);
            elf_file.read(segment_data.data(), phdr.p_filesz);

            // Write the segment into memory
            for (size_t i = 0; i < segment_data.size(); i += 4) {
                std::uint32_t word = 0;
                std::memcpy(&word, &segment_data[i], std::min<size_t>(4, segment_data.size() - i));
                bus_write32(phdr.p_vaddr + i, word);
            }
        }

        // Set the entry point
        set_pc<T>(header.e_entry);
    } else if constexpr (sizeof(T) == 8) {
        // Read ELF header
        Elf64_Ehdr header;
        elf_file.read(reinterpret_cast<char*>(&header), sizeof(header));

        // Verify ELF magic number
        if (memcmp(header.e_ident, ELFMAG, SELFMAG) != 0) {
            Logger::error("load_elf: " + filename + " is not a valid ELF file");
            return false;
        }

        // Load program headers
        elf_file.seekg(header.e_phoff, std::ios::beg);
        std::vector<Elf64_Phdr> program_headers(header.e_phnum);
        elf_file.read(reinterpret_cast<char*>(program_headers.data()), header.e_phnum * sizeof(Elf64_Phdr));

        // Load sections into memory
        for (const auto& phdr : program_headers) {
            if (phdr.p_type != PT_LOAD) {
                continue;
            }

            // Read the segment from the file
            std::vector<char> segment_data(phdr.p_filesz);
            elf_file.seekg(phdr.p_offset, std::ios::beg);
            elf_file.read(segment_data.data(), phdr.p_filesz);

            // Write the segment into memory
            for (size_t i = 0; i < segment_data.size(); i += 8) {
                std::uint64_t word = 0;
                std::memcpy(&word, &segment_data[i], std::min<size_t>(8, segment_data.size() - i));
                bus_write32(phdr.p_vaddr + i, word);
            }
        }

        // Set the entry point
        set_pc<T>(header.e_entry);
    }

    return true;
}

// Explicit template instantiation
template void Core::load_binary<std::uint32_t>(const std::string& filePathName);
template void Core::load_binary<std::uint64_t>(const std::string& filePathName);
template bool Core::load_elf<std::uint32_t>(const std::string& filename);
template bool Core::load_elf<std::uint64_t>(const std::string& filename);
