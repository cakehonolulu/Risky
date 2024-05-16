#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <unordered_map>

class Disassembler {
public:
    std::string Disassemble(uint32_t opcode, const std::vector<std::string> *regnames);

private:
    std::string DecodeRV16(uint16_t opcode, const std::vector<std::string> *regnames);
    std::string DecodeRV32(uint32_t opcode, const std::vector<std::string> *regnames);
    std::string get_csr_name(uint16_t csr);
};
