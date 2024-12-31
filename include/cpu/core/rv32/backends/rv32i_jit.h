#pragma once

#include <cpu/core/backend.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <memory>
#include <unordered_map>
#include <vector>

class RV32I;

class RV32IJIT : public CoreBackend, public JITBackend {
public:
    RV32IJIT(RV32I* core);
    ~RV32IJIT();
    void execute_opcode(std::uint32_t opcode) override;
    void step() override;

    const std::unordered_map<uint32_t, CompiledBlock>& get_block_cache() const override {
        return block_cache;
    }

private:
    static constexpr size_t CACHE_SIZE = 1024;
    std::unordered_map<uint32_t, CompiledBlock> block_cache;
    std::vector<uint32_t> lru_queue;
    uint64_t execution_count = 0;

    CompiledBlock* compile_block(uint32_t pc);
    void link_blocks();
    void evict_oldest_block();
    CompiledBlock* find_block(uint32_t pc);
    std::tuple<bool, uint32_t, bool> generate_ir_for_opcode(uint32_t opcode, uint32_t current_pc);

    RV32I* core;
    bool ready{false};
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    std::unique_ptr<llvm::ExecutionEngine> executionEngine;

    void no_ext(std::string extension);
    void unknown_rv16_opcode(std::uint16_t opcode);
    void unknown_rv32_opcode(std::uint32_t opcode);
    void unknown_zicsr_opcode(std::uint8_t funct3);

    void rv32i_csrrs(std::uint32_t opcode, uint32_t& current_pc, RV32I* core);
};