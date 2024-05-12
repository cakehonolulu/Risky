#pragma once

#include <risky.h>
#include <cpu/riscv.h>
#include <cpu/core/rv32i.h>
#include <cpu/core/rv32e.h>
#include <cpu/core/rv64i.h>
#include <memory>

class ImGui_Risky : public Risky {
public:
	void init() override;
	void run() override;

private:
	std::unique_ptr<RISCV<32>> riscv_core_32;
	std::unique_ptr<RISCV<64>> riscv_core_64;
	std::unique_ptr<RISCV<32, true>> riscv_core_32e;
};
