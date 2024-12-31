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
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <sstream>

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
        if (module) {
            executionEngine->removeModule(module.get());
            module.reset();
        }
        executionEngine.reset();
    }

    if (context) {
        context.reset();
    }
}

void RV32IJIT::step() {
    single_instruction_mode = true;
    std::uint32_t opcode = core->fetch_opcode();
    execute_opcode(opcode);
    core->registers[0] = 0;
    core->pc += 4;
    single_instruction_mode = false;
}

void RV32IJIT::run() {
    std::uint32_t opcode = core->fetch_opcode();
    execute_opcode(opcode);
    core->registers[0] = 0;
    core->pc += 4;
}

void RV32IJIT::execute_opcode(std::uint32_t opcode) {
    uint32_t pc = core->pc;
    
    // Try to find existing block
    Logger::info("Trying to search for block at PC: " + format("0x{:08X}", pc));
    CompiledBlock* block = find_block(pc);
    
    if (!block) {
        Logger::info("Block not found, compiling new block at PC: " + format("0x{:08X}", pc));
        // Compile new block if not found
        block = compile_block(pc, single_instruction_mode);
        if (!block) {
            Logger::error("Failed to compile block at PC: " + format("0x{:08X}", pc));
            return;
        }
        
        // Add to cache
        if (block_cache.size() >= CACHE_SIZE) {
            evict_oldest_block();
        }
        block_cache[pc] = *block;
    }
    
    // Execute block
    block->last_used = ++execution_count;
    auto exec_fn = (int (*)())block->code_ptr;
    int result = exec_fn();
    
    Logger::info("Executed block at PC " + format("0x{:08X}", pc));
}

CompiledBlock* RV32IJIT::compile_block(uint32_t start_pc, bool single_instruction) {
    uint32_t current_pc = start_pc;
    uint32_t end_pc = start_pc; // Initialize end_pc to start_pc
    
    // Create new module for this block
    auto new_module = std::make_unique<llvm::Module>(
        "block_" + std::to_string(start_pc), *context);
    
    if (!new_module) {
        Logger::error("Failed to create LLVM module");
        return nullptr;
    }
        
    // Create function and basic block
    llvm::FunctionType *funcType = llvm::FunctionType::get(builder->getInt32Ty(), false);
    llvm::Function *func = llvm::Function::Create(funcType, 
                                                llvm::Function::ExternalLinkage,
                                                "exec_" + std::to_string(start_pc),
                                                new_module.get());

    llvm::BasicBlock *entry = llvm::BasicBlock::Create(*context, "entry", func);
    builder->SetInsertPoint(entry);

    while (true) {
        uint32_t opcode = core->fetch_opcode(current_pc);
        
        // Generate IR for opcode
        auto [is_branch, current_pc_, error] = generate_ir_for_opcode(opcode, current_pc);

        if (error) {
            Logger::error("Error generating IR for opcode");
            return nullptr;
        }

        // Check if opcode is a branch or jump
        if (is_branch || single_instruction) {
            end_pc = current_pc_;
            break;
        }

        current_pc = current_pc_;

        sleep(1);
    }

    builder->CreateRetVoid();

    std::string str;
    llvm::raw_string_ostream os(str);
    new_module->print(os, nullptr);
    os.flush();

    // Add module to execution engine
    if (!executionEngine) {
        Logger::error("Execution engine is not initialized");
        return nullptr;
    }

    executionEngine->addModule(std::move(new_module));
    executionEngine->finalizeObject();
    
    auto exec_fn = (void (*)())executionEngine->getPointerToFunction(func);
    if (!exec_fn) {
        Logger::error("Failed to JIT compile function");
        return nullptr;
    }

    // Create and return compiled block
    auto block = new CompiledBlock();
    block->start_pc = start_pc;
    block->end_pc = end_pc;
    block->code_ptr = (void*)exec_fn;
    block->last_used = execution_count;
    block->contains_branch = false;
    block->llvm_ir = str;

    return block;
}

CompiledBlock* RV32IJIT::find_block(uint32_t pc) {
    auto it = block_cache.find(pc);
    if (it != block_cache.end()) {
        return &it->second;
    }
    return nullptr;
}

void RV32IJIT::evict_oldest_block() {
    if (lru_queue.empty()) return;
    uint32_t oldest_pc = lru_queue.front();
    lru_queue.erase(lru_queue.begin());
    block_cache.erase(oldest_pc);
}


void RV32IJIT::no_ext(std::string extension) {
	std::ostringstream logMessage;
	logMessage << "FATAL ERROR: Called a " << extension.c_str() << " extension opcode but it's unavailable for this core!";

	Logger::error(logMessage.str());

	Risky::exit(1, Risky::Subsystem::Core);
}

void RV32IJIT::unknown_rv16_opcode(std::uint16_t opcode) {
    std::ostringstream logMessage;
    logMessage << "Unimplemented LLVM IR RV16 Opcode: 0x" << format("{:04X}", opcode);

    Logger::error(logMessage.str());

    Risky::exit(1, Risky::Subsystem::Core);
}

void RV32IJIT::unknown_rv32_opcode(std::uint32_t opcode) {
    std::ostringstream logMessage;
    logMessage << "Unimplemented LLVM IR RV32 Opcode: 0x" << format("{:08X}", opcode);

    Logger::error(logMessage.str());

    Risky::exit(1, Risky::Subsystem::Core);
}

void RV32IJIT::unknown_zicsr_opcode(std::uint8_t funct3) {
	std::ostringstream logMessage;
	logMessage << "Unimplemented LLVM IR Zicsr opcode: 0b" << format("{:08b}", funct3);

	Logger::error(logMessage.str());

	Risky::exit(1, Risky::Subsystem::Core);
}

std::tuple<bool, uint32_t, bool> RV32IJIT::generate_ir_for_opcode(uint32_t opcode, uint32_t current_pc) {
    bool is_branch = false;
    bool error = false;

    std::uint8_t opcode_rv32 = 0;
    std::uint16_t opcode_ = 0;
    std::uint16_t opcode_rv16 = 0;
    std::uint8_t funct3 = 0;
    std::uint32_t funct7 = 0;

    // Check for RV16
    if ((opcode & 0x3) != 0x3) {
        opcode_ = static_cast<std::uint16_t>(opcode);
        opcode_rv16 = (opcode_ >> 0) & 0x3;
        funct3 = (opcode_rv16 >> 13) & 0x7;

        switch (opcode_rv16) {
            case 1:
                switch (funct3) {
                    default:
                        std::ostringstream logMessage;
                        logMessage << "Unimplemented LLVM IR RV16 Opcode: 0x" << format("{:04X}", opcode_rv16) << ", funct3: 0b" << format("{:04b}", funct3);

                        Logger::error(logMessage.str());

                        Risky::exit(1, Risky::Subsystem::Core);
                        
                        error = true;
                        break;
                }
                break;

            default:
                unknown_rv16_opcode(opcode_rv16);
                error = true;
                break;
        }
    }
    else
    {
        // RV32
        funct3 = (opcode >> 12) & 0x7;
        funct7 = (opcode >> 25) & 0x7F;
        opcode_rv32 = opcode & 0x7F;

        switch (opcode_rv32) {
            case SYSTEM:
                if (core->has_zicsr)
                {
                    switch (funct3) {
                        case 0b010:
                            rv32i_csrrs(opcode, current_pc, core);
                            break;

                        default:
                            unknown_zicsr_opcode(funct3);
                            error = true;
                            break;
                    }
                }
                else
                {
                    no_ext("Zicsr");
                    error = true;
                }

                break;

            default:
                unknown_rv32_opcode(opcode);
                error = true;
                break;
        }
    }

    return {is_branch, current_pc, error};
}

void RV32IJIT::rv32i_csrrs(std::uint32_t opcode, uint32_t& current_pc, RV32I* core) {
    uint8_t rd = (opcode >> 7) & 0x1F;
    uint8_t rs1 = (opcode >> 15) & 0x1F;
    uint16_t csr = (opcode >> 20) & 0xFFF;

    // Load the value from the CSR register
    llvm::Value *csr_val = builder->getInt32(core->csr_read(csr));

    // Get the base pointer for the registers array
    llvm::Value *registers_base_ptr = builder->CreateIntToPtr(
        builder->getInt64(reinterpret_cast<std::uintptr_t>(core->registers)),
        llvm::PointerType::getUnqual(builder->getInt32Ty()));

    // Load the value from the source register
    llvm::Value *rs1_ptr = builder->CreateGEP(builder->getInt32Ty(), registers_base_ptr, builder->getInt32(rs1));
    llvm::Value *rs1_val = builder->CreateLoad(builder->getInt32Ty(), rs1_ptr);

    // Perform the bitwise OR operation
    llvm::Value *result = builder->CreateOr(csr_val, rs1_val);

    // Store the result in the destination register
    llvm::Value *rd_ptr = builder->CreateGEP(builder->getInt32Ty(), registers_base_ptr, builder->getInt32(rd));
    builder->CreateStore(result, rd_ptr);

    // Update the CSR register if rs1 is not zero
    llvm::Value *rs1_is_not_zero = builder->CreateICmpNE(rs1_val, builder->getInt32(0));
    llvm::BasicBlock *update_csr_block = llvm::BasicBlock::Create(*context, "update_csr", builder->GetInsertBlock()->getParent());
    llvm::BasicBlock *continue_block = llvm::BasicBlock::Create(*context, "continue", builder->GetInsertBlock()->getParent());
    builder->CreateCondBr(rs1_is_not_zero, update_csr_block, continue_block);

    builder->SetInsertPoint(update_csr_block);
    // Ensure that csr_write is called correctly
    llvm::FunctionType *csr_write_type = llvm::FunctionType::get(builder->getVoidTy(), {builder->getInt32Ty(), builder->getInt32Ty()}, false);
    llvm::FunctionCallee csr_write_func = builder->GetInsertBlock()->getModule()->getOrInsertFunction("csr_write", csr_write_type);
    
    builder->CreateCall(csr_write_func, {builder->getInt32(csr), result});
    builder->CreateBr(continue_block);

    builder->SetInsertPoint(continue_block);

    current_pc += 4;
}