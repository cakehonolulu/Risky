#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <unordered_map>

class Disassembler {
public:
    std::string Disassemble(uint32_t opcode);

private:
    std::string DecodeRV16(uint16_t opcode);
    std::string DecodeRV32(uint32_t opcode);
    std::string get_csr_name(uint16_t csr);
};
