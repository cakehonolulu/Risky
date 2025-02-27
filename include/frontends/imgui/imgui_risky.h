#pragma once

#include <risky.h>
#include <cpu/riscv.h>
#include <cpu/core/rv32/rv32i.h>
#include <cpu/core/rv32/rv32e.h>
#include <cpu/core/rv64/rv64i.h>
#include <cpu/core/core.h>
#include <utils/symbols.h>
#include <memory>
#include "imgui.h"
#include "log/log_imgui.hh"

struct DisplayItem {
	std::string text;
	std::string opcodeBuffer;
	bool isCurrentInstruction;
	bool isJumpInstruction;
	bool hasSymbol;
	ImVec4 color;
};

class ImGui_Risky : public Risky {
public:
	ImGui_Risky();
	void init() override;
	void run() override;

    void imgui_registers_window_32(Core *core, bool *debug_window);
    void imgui_registers_window_64(Core *core, bool *debug_window);
    void imgui_disassembly_window_32(Core *core);
	void imgui_llvm_ir_blocks_window(Core *core);

private:
	std::unique_ptr<RV32I> riscv_core_32;
    std::unique_ptr<RV32E> riscv_core_32e;
	std::unique_ptr<RV64I> riscv_core_64;

    std::unordered_map<std::uint32_t, Symbol> symbols;
	bool symbols_loaded;
	std::shared_ptr<ImGuiLogBackend> imgui_logger;
};
