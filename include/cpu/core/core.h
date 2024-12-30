#pragma once

enum class EmulationType {
    Interpreter,
    JIT
};

#include <functional>
#include <any>
#include <cstdint>
#include <type_traits>
#include <elf.h>
#include <utils/core_thread.h>
#include <cpu/riscv.h>
#include <cpu/core/rv32/rv32i.h>
#include <cpu/core/rv32/rv32e.h>
#include <cpu/core/rv64/rv64i.h>
#include <cpu/core/rv32/backends/rv32i_interpreter.h>
#include <cpu/core/rv32/backends/rv32i_jit.h>

// Define Core class
class Core {
public:
    Core() : riscv(nullptr), xlen(0), is_embedded(false), emulationType(EmulationType::Interpreter) {}

    // Function pointers for pc, registers, and csrs
    std::function<std::any()> pc;
    std::function<std::any(size_t)> registers;
    std::function<std::any(size_t)> csrs;
    std::function<std::uint32_t(std::uint32_t)> bus_read32;
    std::function<void(std::uint32_t, std::uint32_t)> bus_write32;
    std::function<void()> step;
    std::function<void()> stop;
    std::function<void()> reset;
    std::function<void()> run;

    // Assign a new RISCV instance to Core
    template <std::uint8_t xlen, bool is_embedded>
    void assign(RISCV<xlen, is_embedded>* riscv, EmulationType emulationType) {
        this->emulationType = emulationType;

        // Assign function pointers based on xlen
        bus_read32 = [riscv](std::uint32_t address) -> std::uint32_t { return std::uint32_t(riscv->bus.read32(address)); };
        bus_write32 = [riscv](std::uint32_t address, std::uint32_t value) { riscv->bus.write32(address, value); };
        step = [riscv]() { riscv->step(); };
        reset = [riscv]() { riscv->reset(); };
        run = [riscv]() { riscv->run(); };
        stop = [riscv]() { riscv->stop(); };

        pc = [riscv]() -> std::any { return std::any(riscv->pc); };
        registers = [riscv](size_t index) -> std::any { return std::any(riscv->registers[index]); };
        csrs = [riscv](size_t index) -> std::any { return std::any(riscv->csrs[index]); };

        // Store the pointer to the RISCV instance and metadata
        this->riscv = riscv;
        this->xlen = xlen;
        this->is_embedded = is_embedded;
    }

    // Set the pc value
    template <typename T>
    void set_pc(T newPC) {
        if (is_embedded) {
            auto riscvInstance = std::any_cast<RISCV<sizeof(T) * 8, true>*>(riscv);
            if (riscvInstance) {
                riscvInstance->pc = newPC;
            } else {
                Logger::error("Incorrect xlen for set_pc");
                Risky::exit(1, Risky::Subsystem::Core);
            }
        } else {
            auto riscvInstance = std::any_cast<RISCV<sizeof(T) * 8, false>*>(riscv);
            if (riscvInstance) {
                riscvInstance->pc = newPC;
            } else {
                Logger::error("Incorrect xlen for set_pc");
                Risky::exit(1, Risky::Subsystem::Core);
            }
        }
    }

    // Get a register value
    template <typename T>
    T get_register(std::size_t idx) const {
        return std::any_cast<T>(registers(idx));
    }

    // Load binary file based on core configuration
    template <typename T>
    void load_binary(const std::string& filePathName);

    // Load ELF file based on core configuration
    template <typename T>
    bool load_elf(const std::string& filename);

    // Get the xlen value
    std::uint8_t get_xlen() const {
        return xlen;
    }

    // Check if Core has an assigned RISCV instance with specific parameters
    bool has_riscv(std::uint8_t xlen, bool is_embedded) const {
        return (this->xlen == xlen) && (this->is_embedded == is_embedded);
    }

    void start_() {
        steppingThread.start([this]() { this->step(); });
    }

    void stop_() {
        steppingThread.stop();
    }

    bool thread_running() const {
        return steppingThread.isRunning();
    }

    bool check_thread() {
        return steppingThread.checkAndClearUpdateFlag();
    }

    EmulationType get_emulation_type() const {
        return emulationType;
    }

    std::any get_riscv() const {
        return riscv;
    }

private:
    // Pointer to the current RISCV instance
    std::any riscv;
    std::uint8_t xlen;
    bool is_embedded;
    EmulationType emulationType;
    SteppingThread steppingThread;
};
