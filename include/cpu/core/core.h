#pragma once

#include <functional>
#include <any>
#include <cstdint>
#include <type_traits>
#include <elf.h>

// Forward declaration of RISCV template class
template <std::uint8_t xlen, bool is_embedded>
class RISCV;

// Define Core class
class Core {
public:
    Core() : riscv(nullptr), xlen(0), is_embedded(false) {}

    // Function pointers for pc, registers, and csrs
    std::function<std::any()> pc;
    std::function<std::any(size_t)> registers;
    std::function<std::any(size_t)> csrs;
    std::function<std::uint32_t(std::uint32_t)> bus_read32;
    std::function<void(std::uint32_t, std::uint32_t)> bus_write32;
    std::function<void()> step;
    std::function<void()> reset;
    std::function<void()> run;

    // Assign a new RISCV instance to Core
    template <std::uint8_t xlen, bool is_embedded>
    void assign(RISCV<xlen, is_embedded>* riscv) {
        // Assign function pointers based on xlen
        bus_read32 = [riscv](std::uint32_t address) -> std::uint32_t { return std::uint32_t(riscv->bus.read32(address)); };
        bus_write32 = [riscv](std::uint32_t address, std::uint32_t value) { riscv->bus.write32(address, value); };
        step = [riscv]() { riscv->step(); };
        reset = [riscv]() { riscv->reset(); };
        run = [riscv]() { riscv->run(); };

        if constexpr (xlen == 32) {
            pc = [riscv]() -> std::any { return std::any(riscv->pc); };
            registers = [riscv](size_t index) -> std::any { return std::any(riscv->registers[index]); };
            csrs = [riscv](size_t index) -> std::any { return std::any(riscv->csrs[index]); };
        } else if constexpr (xlen == 64) {
            pc = [riscv]() -> std::any { return std::any(riscv->pc); };
            registers = [riscv](size_t index) -> std::any { return std::any(riscv->registers[index]); };
            csrs = [riscv](size_t index) -> std::any { return std::any(riscv->csrs[index]); };
        }

        // Store the pointer to the RISCV instance and metadata
        this->riscv = riscv;
        this->xlen = xlen;
        this->is_embedded = is_embedded;
    }

    // Set the pc value for xlen = 32
    void set_pc_32(std::uint32_t newPC) {
        if (xlen == 32) {
            auto riscvInstance = std::any_cast<RISCV<32, false>*>(riscv);
            if (riscvInstance) {
                riscvInstance->pc = newPC;
            }
        } else {
            Logger::Instance().Error("Incorrect xlen for set_pc_32");
            Risky::exit();
        }
    }

    // Set the pc value for xlen = 64
    void set_pc_64(std::uint64_t newPC) {
        if (xlen == 64) {
            auto riscvInstance = std::any_cast<RISCV<64, false>*>(riscv);
            if (riscvInstance) {
                riscvInstance->pc = newPC;
            }
        } else {
            Logger::Instance().Error("Incorrect xlen for set_pc_64");
            Risky::exit();
        }
    }

    // Get a 32-bit register value
    std::uint32_t register_32(std::size_t idx) const {
        if (xlen != 32) {
            Logger::Instance().Error("Incompatible register access (expected 32-bit)");
            Risky::exit();
        }

        std::uint32_t register_ = std::any_cast<std::uint32_t>(registers(idx));

        return register_;
    }

    // Get a 64-bit register value
    std::uint64_t register_64(std::size_t idx) const {
        if (xlen != 64) {
            Logger::Instance().Error("Incompatible register access (expected 64-bit)");
            Risky::exit();
        }

        std::uint64_t register_ = std::any_cast<std::uint64_t>(registers(idx));

        return register_;
    }

    // Load binary file based on core configuration
    void load_binary(const std::string& filePathName) {
        if (riscv.has_value()) {
            if (xlen == 32) {
                auto riscv_32 = std::any_cast<RISCV<32, false>*>(&riscv);
                if (riscv_32) {
                    (*riscv_32)->bus.load_binary(filePathName);
                } else {
                    Logger::Instance().Error("Incompatible RISCV instance (expected 32-bit)");
                    Risky::exit();
                }
            } else if (xlen == 64) {
                auto riscv_64 = std::any_cast<RISCV<64, false>*>(&riscv);
                if (riscv_64) {
                    (*riscv_64)->bus.load_binary(filePathName);
                } else {
                    Logger::Instance().Error("Incompatible RISCV instance (expected 64-bit)");
                    Risky::exit();
                }
            } else {
                Logger::Instance().Error("Unsupported xlen");
                Risky::exit();
            }
        } else {
            Logger::Instance().Error("No RISCV instance assigned to Core");
            Risky::exit();
        }
    }

    // Load ELF file based on core configuration
    bool load_elf(const std::string& filename) {
        std::ifstream elf_file(filename, std::ios::binary);
        if (!elf_file) {
            Logger::Instance().Error("[RISKY] load_elf: Could not open ELF file " + filename);
            return false;
        }

        // Read ELF header
        Elf32_Ehdr header;
        elf_file.read(reinterpret_cast<char*>(&header), sizeof(header));

        // Verify ELF magic number
        if (memcmp(header.e_ident, ELFMAG, SELFMAG) != 0) {
            Logger::Instance().Error("[RISKY] load_elf: " + filename + " is not a valid ELF file");
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
        set_pc_32(header.e_entry);

        return true;
    }

    // Get the xlen value
    std::uint8_t get_xlen() const {
        return xlen;
    }

    // Check if Core has an assigned RISCV instance with specific parameters
    bool has_riscv(std::uint8_t xlen, bool is_embedded) const {
        return (this->xlen == xlen) && (this->is_embedded == is_embedded);
    }

private:
    // Pointer to the current RISCV instance
    std::any riscv;
    std::uint8_t xlen;
    bool is_embedded;
};
