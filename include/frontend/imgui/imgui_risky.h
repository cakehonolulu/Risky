#pragma once

#include <risky.h>
#include <cpu/riscv.h>
#include <cpu/core/rv32i.h>
#include <cpu/core/rv32e.h>
#include <cpu/core/rv64i.h>
#include <utils/symbols.h>
#include <memory>

class ImGui_Risky : public Risky {
public:
	void init() override;
	void run() override;

private:
	std::unique_ptr<RV32I> riscv_core_32;
    std::unique_ptr<RV32E> riscv_core_32e;
	std::unique_ptr<RV64I> riscv_core_64;

    std::unordered_map<std::uint32_t, Symbol> symbols;
};
