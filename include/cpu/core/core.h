#pragma once

#include <functional>
#include <any>
#include <cstdint>
#include <type_traits> // For std::integral_constant

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

    // Assign a new RISCV instance to Core
    template <std::uint8_t xlen, bool is_embedded>
    void assign(RISCV<xlen, is_embedded>* riscv) {
        // Assign function pointers based on xlen
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

    // Set the pc value with a templated function
    template <std::uint8_t xlen>
    void set_pc(std::conditional_t<xlen == 32, std::uint32_t, std::uint64_t> newPC) {
        if (this->xlen == xlen) {
            if constexpr (xlen == 32) {
                auto riscvInstance = std::any_cast<RISCV<32, false>*>(riscv);
                if (riscvInstance) {
                    riscvInstance->pc = newPC;
                }
            } else if constexpr (xlen == 64) {
                auto riscvInstance = std::any_cast<RISCV<64, false>*>(riscv);
                if (riscvInstance) {
                    riscvInstance->pc = newPC;
                }
            }
        } else {
            throw std::runtime_error("Incorrect xlen for setPC");
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
