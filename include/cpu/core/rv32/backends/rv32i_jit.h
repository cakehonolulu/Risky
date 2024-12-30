#pragma once

#include <cpu/core/backend.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <memory>

class RV32I;

class RV32IJIT : public CoreBackend {
public:
    RV32IJIT(RV32I* core);
    ~RV32IJIT();
    void execute_opcode(std::uint32_t opcode) override;
    void step();

private:
    RV32I* core;
    bool ready{false};
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    std::unique_ptr<llvm::ExecutionEngine> executionEngine;
};