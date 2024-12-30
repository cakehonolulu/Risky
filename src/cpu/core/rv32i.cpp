#include <cpu/core/rv32i.h>
#include <cpu/core/rv32i_interpreter.h>
#include <cpu/core/rv32i_jit.h>
#include <bitset>

RV32I::RV32I(const std::vector<std::string>& extensions, EmulationType emulationType)
        : RISCV<32>(extensions), emulationType(emulationType) {
    if (emulationType == EmulationType::Interpreter) {
        interpreter = std::make_unique<RV32IInterpreter>(extensions, &bus);
        set_step_func([this] { interpreter->step(); });
    } else {
        jit = std::make_unique<RV32IJIT>(extensions);
        set_step_func([this] { jit->step(); });
    }
}

void RV32I::execute_opcode(std::uint32_t opcode) {
    if (emulationType == EmulationType::Interpreter) {
        interpreter->execute_opcode(opcode);
    } else {
        jit->execute_opcode(opcode);
    }
}

void RV32I::step() {
    if (emulationType == EmulationType::Interpreter) {
        interpreter->step();
    } else {
        jit->step();
    }
}

void RV32I::no_ext(std::string extension) {
    if (emulationType == EmulationType::Interpreter) {
        interpreter->no_ext(extension);
    } else {
        //jit->no_ext(extension);
    }
}

void RV32I::unknown_rv16_opcode(std::uint16_t opcode) {
    if (emulationType == EmulationType::Interpreter) {
        interpreter->unknown_rv16_opcode(opcode);
    } else {
        //jit->unknown_rv16_opcode(opcode);
    }
}

void RV32I::unknown_rv32_opcode(std::uint32_t opcode) {
    if (emulationType == EmulationType::Interpreter) {
        interpreter->unknown_rv32_opcode(opcode);
    } else {
        //jit->unknown_rv32_opcode(opcode);
    }
}

void RV32I::unknown_zicsr_opcode(std::uint8_t funct3) {
    if (emulationType == EmulationType::Interpreter) {
        interpreter->unknown_zicsr_opcode(funct3);
    } else {
        //jit->unknown_zicsr_opcode(funct3);
    }
}

void RV32I::unknown_load_opcode(std::uint8_t funct3) {
    if (emulationType == EmulationType::Interpreter) {
        interpreter->unknown_load_opcode(funct3);
    } else {
        //jit->unknown_load_opcode(funct3);
    }
}

void RV32I::unknown_miscmem_opcode(std::uint8_t funct3) {
    if (emulationType == EmulationType::Interpreter) {
        interpreter->unknown_miscmem_opcode(funct3);
    } else {
        //jit->unknown_miscmem_opcode(funct3);
    }
}

void RV32I::unknown_branch_opcode(std::uint8_t funct3) {
    if (emulationType == EmulationType::Interpreter) {
        interpreter->unknown_branch_opcode(funct3);
    } else {
        //jit->unknown_branch_opcode(funct3);
    }
}

void RV32I::unknown_store_opcode(std::uint8_t funct3) {
    if (emulationType == EmulationType::Interpreter) {
        interpreter->unknown_store_opcode(funct3);
    } else {
        //jit->unknown_store_opcode(funct3);
    }
}

void RV32I::unknown_amo_opcode(std::uint8_t funct3, std::uint8_t funct7) {
    if (emulationType == EmulationType::Interpreter) {
        interpreter->unknown_amo_opcode(funct3, funct7);
    } else {
        //jit->unknown_amo_opcode(funct3, funct7);
    }
}

void RV32I::unknown_op_opcode(std::uint8_t funct3, std::uint8_t funct7) {
    if (emulationType == EmulationType::Interpreter) {
        interpreter->unknown_op_opcode(funct3, funct7);
    } else {
        //jit->unknown_op_opcode(funct3, funct7);
    }
}

void RV32I::unknown_immediate_opcode(std::uint8_t funct3) {
    if (emulationType == EmulationType::Interpreter) {
        interpreter->unknown_immediate_opcode(funct3);
    } else {
        //jit->unknown_immediate_opcode(funct3);
    }
}

void RV32I::unknown_compressed_opcode(std::uint8_t funct3) {
    if (emulationType == EmulationType::Interpreter) {
        interpreter->unknown_compressed_opcode(funct3);
    } else {
        //jit->unknown_compressed_opcode(funct3);
    }
}
