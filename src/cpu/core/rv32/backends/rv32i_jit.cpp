#include <cpu/core/rv32/backends/rv32i_jit.h>
#include <cpu/core/rv32/rv32i.h>
#include <log/log.hh>
#include <risky.h>
#include <cpu/core/core.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/Support/TargetSelect.h>

RV32IJIT::RV32IJIT(RV32I* core) : core(core) {
    // Initialize LLVM components
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    // Create context and builder
    context = std::make_unique<llvm::LLVMContext>();
    builder = std::make_unique<llvm::IRBuilder<>>(*context);

    // Create module
    module = std::make_unique<llvm::Module>("rv32i_jit", *context);
    
    // Create execution engine with module ownership
    std::string errStr;
    llvm::Module* modulePtr = module.get(); // Keep raw pointer
    
    executionEngine = std::unique_ptr<llvm::ExecutionEngine>(
        llvm::EngineBuilder(std::unique_ptr<llvm::Module>(modulePtr))
            .setErrorStr(&errStr)
            .setEngineKind(llvm::EngineKind::JIT)
            .create());

    if (!executionEngine) {
        Logger::error("Failed to create ExecutionEngine: " + errStr);
        return;
    }

    Logger::info("JIT initialization successful");
    ready = true;
}

RV32IJIT::~RV32IJIT() {
    if (builder) {
        builder.reset();
    }

    if (executionEngine) {
        executionEngine->removeModule(module.get());
        executionEngine.reset();
    }

    if (module) {
        module->dropAllReferences();
        module.reset();
    }

    if (context) {
        context.reset();
    }
}

void RV32IJIT::execute_opcode(std::uint32_t opcode) {
    if (!ready || !context || !builder || !executionEngine) {
        Logger::error("JIT not properly initialized");
        return;
    }

    // Decode RV32I instruction fields
    uint32_t funct7 = (opcode >> 25) & 0x7f;
    uint32_t rs2 = (opcode >> 20) & 0x1f;
    uint32_t rs1 = (opcode >> 15) & 0x1f;
    uint32_t funct3 = (opcode >> 12) & 0x7;
    uint32_t rd = (opcode >> 7) & 0x1f;
    uint32_t opcode_field = opcode & 0x7f;

    if (module) {
        executionEngine->removeModule(module.get());
        module.reset();
    }
    
    module = std::make_unique<llvm::Module>("rv32i_jit_" + std::to_string(opcode), *context);
    
    // Create function and basic block
    llvm::FunctionType *funcType = llvm::FunctionType::get(builder->getInt32Ty(), false);
    llvm::Function *func = llvm::Function::Create(funcType, 
                                                llvm::Function::ExternalLinkage,
                                                "exec_" + std::to_string(opcode),
                                                module.get());

    llvm::BasicBlock *entry = llvm::BasicBlock::Create(*context, "entry", func);
    builder->SetInsertPoint(entry);

    switch (opcode_field) {
        // TODO: Implement actual RV32I instructions
        default:
            builder->CreateRet(builder->getInt32(0));
            break;
    }

    if (!module) {
        Logger::error("Module is null before print operation");
        return;
    }

    // JIT compile and execute
    module->print(llvm::errs(), nullptr); // Debug output
    // Add module to execution engine
    executionEngine->addModule(std::move(module));
    executionEngine->finalizeObject();
    
    auto execFunc = (int (*)())executionEngine->getPointerToFunction(func);
    if (execFunc) {
        int result = execFunc();
        Logger::info("JIT function returned: " + std::to_string(result));
    } else {
        Logger::error("Failed to JIT compile function");
    }
}
