#pragma once

#include <cpu/core/core.h>
#include <cpu/core/backend.h>
#include <cstdint>
#include <string>

class RV32I;

class RV32IInterpreter : public CoreBackend {
public:
    RV32IInterpreter(RV32I* core);
    void execute_opcode(std::uint32_t opcode) override;
    void step() override;
    void run() override;

private:
    RV32I* core;

    void rv32i_caddi(std::uint16_t opcode);
    void rv32i_auipc(std::uint32_t opcode);
    void rv32i_lui(std::uint32_t opcode);
    void rv32i_jal(std::uint32_t opcode);
    void rv32i_jalr(std::uint32_t opcode);
    void rv32i_lb(std::uint32_t opcode);
    void rv32i_lw(std::uint32_t opcode);
    void rv32i_lbu(std::uint32_t opcode);
    void rv32i_sub(std::uint32_t opcode);
    void rv32i_add(std::uint32_t opcode);
    void rv32i_sll(std::uint32_t opcode);
    void rv32i_sltu(std::uint32_t opcode);
    void rv32i_xor(std::uint32_t opcode);
    void rv32i_or(std::uint32_t opcode);
    void rv32i_and(std::uint32_t opcode);
    void rv32i_mul(std::uint32_t opcode);
    void rv32i_mulhu(std::uint32_t opcode);
    void rv32i_div(std::uint32_t opcode);
    void rv32i_csrrw(std::uint32_t opcode);
    void rv32i_csrrs(std::uint32_t opcode);
    void rv32i_csrrc(std::uint32_t opcode);
    void rv32i_csrrsi(std::uint32_t opcode);
    void rv32i_csrrwi(std::uint32_t opcode);
    void rv32i_fence_i(std::uint32_t opcode);
    void rv32i_addi(std::uint32_t opcode);
    void rv32i_slli(std::uint32_t opcode);
    void rv32i_sltiu(std::uint32_t opcode);
    void rv32i_srli(std::uint32_t opcode);
    void rv32i_andi(std::uint32_t opcode);
    void rv32i_beq(std::uint32_t opcode);
    void rv32i_bne(std::uint32_t opcode);
    void rv32i_blt(std::uint32_t opcode);
    void rv32i_bge(std::uint32_t opcode);
    void rv32i_bltu(std::uint32_t opcode);
    void rv32i_bgeu(std::uint32_t opcode);
    void rv32i_sb(std::uint32_t opcode);
    void rv32i_sw(std::uint32_t opcode);
    void rv32i_amoor_w(std::uint32_t opcode);
    void rv32i_amoadd_w(std::uint32_t opcode);

    void no_ext(std::string extension);
    void unknown_rv16_opcode(std::uint16_t opcode);
    void unknown_rv32_opcode(std::uint32_t opcode);
    void unknown_zicsr_opcode(std::uint8_t funct3);
    void unknown_load_opcode(std::uint8_t funct3);
    void unknown_miscmem_opcode(std::uint8_t funct3);
    void unknown_branch_opcode(std::uint8_t funct3);
    void unknown_store_opcode(std::uint8_t funct3);
    void unknown_amo_opcode(std::uint8_t funct3, std::uint8_t funct7);
    void unknown_op_opcode(std::uint8_t funct3, std::uint8_t funct7);
    void unknown_immediate_opcode(std::uint8_t funct3);
    void unknown_compressed_opcode(std::uint8_t funct3);
};
