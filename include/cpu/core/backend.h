#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

class CompiledBlock {
public:
    uint32_t start_pc;
    uint32_t end_pc;
    void* code_ptr;
    uint64_t last_used;
    bool contains_branch;
    std::string llvm_ir;
};

class JITBackend {
public:
    virtual ~JITBackend() = default;
    virtual const std::unordered_map<uint32_t, CompiledBlock>& get_block_cache() const = 0;
};

class CoreBackend {
public:
    virtual ~CoreBackend() = default;
    virtual void execute_opcode(std::uint32_t opcode) = 0;
    virtual void step() = 0;
    virtual void run() = 0;

    bool ready = false;
};
