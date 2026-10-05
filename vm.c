#include "vm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Single definition for shared tables (was static per-TU before)
const char *const reg_names[32] = {
    "zero","ra","sp","gp","tp","t0","t1","t2",
    "s0","s1","a0","a1","a2","a3","a4","a5","a6","a7",
    "s2","s3","s4","s5","s6","s7","s8","s9","s10","s11",
    "t3","t4","t5","t6"
};

const int palette_256[16] = {
    16, 15,  88, 159,  127,  34,  19, 227,
    208, 94, 210, 242, 245, 120,  27, 250
};

bool cpu_load_bin(CPU *cpu, const char *path) {
    auto f = fopen(path, "rb");
    if (f == nullptr) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz > (long)(MEM_SIZE - PROG_BASE)) sz = (long)(MEM_SIZE - PROG_BASE);
    if (sz < 0) sz = 0;
    memset(cpu->mem, 0, MEM_SIZE);
    size_t n = fread(cpu->mem + PROG_BASE, 1, (size_t)sz, f);
    fclose(f);
    cpu->prog_size = n;
    cpu->pc = PROG_BASE;
    memset(cpu->regs, 0, sizeof(cpu->regs));
    cpu->halted = false;
    cpu->last_key = 0;
    return true;
}

void cpu_reset(CPU *cpu) {
    cpu->pc = PROG_BASE;
    memset(cpu->regs, 0, sizeof(cpu->regs));
    cpu->halted = false;
    cpu->last_key = 0;
}

uint32_t cpu_fetch(const CPU *cpu) {
    if (cpu->pc + 3 >= MEM_SIZE) return 0;
    return (uint32_t)cpu->mem[cpu->pc]
         | (uint32_t)cpu->mem[cpu->pc + 1] << 8
         | (uint32_t)cpu->mem[cpu->pc + 2] << 16
         | (uint32_t)cpu->mem[cpu->pc + 3] << 24;
}

[[nodiscard]] static inline int32_t sign_extend(uint32_t val, int bits) {
    int32_t m = (int32_t)(1u << (bits - 1));
    return (int32_t)((val ^ (uint32_t)m) - (uint32_t)m);
}

const char *decode_to_str(uint32_t instr, uint32_t pc, char *out, size_t outlen) {
    uint32_t opcode = instr & 0x7F;
    uint32_t rd     = (instr >> 7) & 0x1F;
    uint32_t funct3 = (instr >> 12) & 0x7;
    uint32_t rs1    = (instr >> 15) & 0x1F;
    uint32_t rs2    = (instr >> 20) & 0x1F;
    uint32_t funct7 = (instr >> 25) & 0x7F;

    if (instr == 0) { snprintf(out, outlen, "unimp"); return out; }

    if (opcode == OP_LUI) {
        int32_t imm = (int32_t)(instr & 0xFFFFF000);
        snprintf(out, outlen, "lui     %s,0x%x", reg_names[rd], (uint32_t)imm >> 12);
        return out;
    }
    if (opcode == OP_AUIPC) {
        int32_t imm = (int32_t)(instr & 0xFFFFF000);
        snprintf(out, outlen, "auipc   %s,0x%x", reg_names[rd], (uint32_t)imm >> 12);
        return out;
    }
    if (opcode == OP_JAL) {
        int32_t imm = (int32_t)(((instr >> 31) & 1) << 20 | ((instr >> 12) & 0xFF) << 12 | ((instr >> 20) & 1) << 11 | ((instr >> 21) & 0x3FF) << 1);
        imm = sign_extend((uint32_t)imm, 21);
        snprintf(out, outlen, "jal     %s,0x%x", reg_names[rd], pc + (uint32_t)imm);
        return out;
    }
    if (opcode == OP_JALR) {
        int32_t imm = sign_extend(instr >> 20, 12);
        snprintf(out, outlen, "jalr    %s,%d(%s)", reg_names[rd], imm, reg_names[rs1]);
        return out;
    }
    if (opcode == OP_BRANCH) {
        int32_t imm = (int32_t)(((instr >> 31) & 1) << 12 | ((instr >> 7) & 1) << 11 | ((instr >> 25) & 0x3F) << 5 | ((instr >> 8) & 0xF) << 1);
        imm = sign_extend((uint32_t)imm, 13);
        const char *mn = "b??";
        if (funct3 == 0) mn = "beq";
        else if (funct3 == 1) mn = "bne";
        else if (funct3 == 4) mn = "blt";
        else if (funct3 == 5) mn = "bge";
        else if (funct3 == 6) mn = "bltu";
        else if (funct3 == 7) mn = "bgeu";
        if (funct3 == 0 && rs2 == 0) snprintf(out, outlen, "beqz    %s,0x%x", reg_names[rs1], pc + (uint32_t)imm);
        else if (funct3 == 1 && rs2 == 0) snprintf(out, outlen, "bnez    %s,0x%x", reg_names[rs1], pc + (uint32_t)imm);
        else snprintf(out, outlen, "%-7s %s,%s,0x%x", mn, reg_names[rs1], reg_names[rs2], pc + (uint32_t)imm);
        return out;
    }
    if (opcode == OP_LOAD) {
        int32_t imm = sign_extend(instr >> 20, 12);
        const char *mn = "lb";
        if (funct3 == 0) mn = "lb"; else if (funct3 == 1) mn = "lh"; else if (funct3 == 2) mn = "lw";
        else if (funct3 == 4) mn = "lbu"; else if (funct3 == 5) mn = "lhu";
        snprintf(out, outlen, "%-7s %s,%d(%s)", mn, reg_names[rd], imm, reg_names[rs1]);
        return out;
    }
    if (opcode == OP_STORE) {
        int32_t imm = (int32_t)(((instr >> 25) << 5) | ((instr >> 7) & 0x1F));
        imm = sign_extend((uint32_t)imm, 12);
        const char *mn = "sb";
        if (funct3 == 0) mn = "sb"; else if (funct3 == 1) mn = "sh"; else if (funct3 == 2) mn = "sw";
        snprintf(out, outlen, "%-7s %s,%d(%s)", mn, reg_names[rs2], imm, reg_names[rs1]);
        return out;
    }
    if (opcode == OP_IMM) {
        int32_t imm = sign_extend(instr >> 20, 12);
        uint32_t shamt = (instr >> 20) & 0x3F;
        if (funct3 == 0) {
            if (rs1 == 0) snprintf(out, outlen, "li      %s,%d", reg_names[rd], imm);
            else if (imm == 0) snprintf(out, outlen, "mv      %s,%s", reg_names[rd], reg_names[rs1]);
            else snprintf(out, outlen, "addi    %s,%s,%d", reg_names[rd], reg_names[rs1], imm);
        } else if (funct3 == 1 && funct7 == 0) snprintf(out, outlen, "slli    %s,%s,%u", reg_names[rd], reg_names[rs1], shamt & 0x1F);
        else if (funct3 == 5) {
            if (funct7 == 0) snprintf(out, outlen, "srli    %s,%s,%u", reg_names[rd], reg_names[rs1], shamt & 0x1F);
            else if (funct7 == 0x20) snprintf(out, outlen, "srai    %s,%s,%u", reg_names[rd], reg_names[rs1], shamt & 0x1F);
            else snprintf(out, outlen, "op-imm  0x%08x", instr);
        } else {
            const char *mn = "op-imm";
            if (funct3 == 2) mn = "slti"; else if (funct3 == 3) mn = "sltiu"; else if (funct3 == 4) mn = "xori"; else if (funct3 == 6) mn = "ori"; else if (funct3 == 7) mn = "andi";
            snprintf(out, outlen, "%-7s %s,%s,%d", mn, reg_names[rd], reg_names[rs1], imm);
        }
        return out;
    }
    if (opcode == OP_OP) {
        const char *mn = "add";
        if (funct3 == 0 && funct7 == 0) mn = "add";
        else if (funct3 == 0 && funct7 == 0x20) mn = "sub";
        else if (funct3 == 1) mn = "sll";
        else if (funct3 == 2) mn = "slt";
        else if (funct3 == 3) mn = "sltu";
        else if (funct3 == 4) mn = "xor";
        else if (funct3 == 5 && funct7 == 0) mn = "srl";
        else if (funct3 == 5 && funct7 == 0x20) mn = "sra";
        else if (funct3 == 6) mn = "or";
        else if (funct3 == 7) mn = "and";
        else mn = "op";
        snprintf(out, outlen, "%-7s %s,%s,%s", mn, reg_names[rd], reg_names[rs1], reg_names[rs2]);
        return out;
    }
    if (opcode == OP_SYSTEM) {
        if (instr == 0x00000073) snprintf(out, outlen, "ecall");
        else if (instr == 0x10200073) snprintf(out, outlen, "sret");
        else snprintf(out, outlen, "system  0x%08x", instr);
        return out;
    }
    snprintf(out, outlen, "unknown 0x%08x", instr);
    return out;
}

bool cpu_step(CPU *cpu) {
    if (cpu->halted) return false;
    cpu->regs[0] = 0;
    // MMIO 0xFE/0xFF is intercepted on LOAD (see OP_LOAD below), never
    // stored in mem[] — the old per-step mem[0xFE]=rand() stomp broke any
    // program whose code reaches 0xFE (e.g. Snake, ~400+ bytes).
    if (cpu->pc + 3 >= MEM_SIZE) { cpu->halted = true; return false; }
    uint32_t instr = cpu_fetch(cpu);
    uint32_t cur_pc = cpu->pc;
    cpu->pc += 4;
    uint32_t opcode = instr & 0x7F;
    uint32_t rd     = (instr >> 7) & 0x1F;
    uint32_t funct3 = (instr >> 12) & 0x7;
    uint32_t rs1    = (instr >> 15) & 0x1F;
    uint32_t rs2    = (instr >> 20) & 0x1F;
    uint32_t funct7 = (instr >> 25) & 0x7F;

    switch (opcode) {
        case OP_LUI: {
            int32_t imm = (int32_t)(instr & 0xFFFFF000);
            if (rd != 0) cpu->regs[rd] = (uint32_t)imm;
            break;
        }
        case OP_AUIPC: {
            int32_t imm = (int32_t)(instr & 0xFFFFF000);
            if (rd != 0) cpu->regs[rd] = cur_pc + (uint32_t)imm;
            break;
        }
        case OP_JAL: {
            int32_t imm = (int32_t)(((instr >> 31) & 1) << 20 | ((instr >> 12) & 0xFF) << 12 | ((instr >> 20) & 1) << 11 | ((instr >> 21) & 0x3FF) << 1);
            imm = sign_extend((uint32_t)imm, 21);
            if (rd != 0) cpu->regs[rd] = cur_pc + 4;
            cpu->pc = cur_pc + (uint32_t)imm;
            break;
        }
        case OP_JALR: {
            int32_t imm = sign_extend(instr >> 20, 12);
            uint32_t ret = cur_pc + 4;
            cpu->pc = (cpu->regs[rs1] + (uint32_t)imm) & ~1u;
            if (rd != 0) cpu->regs[rd] = ret;
            break;
        }
        case OP_BRANCH: {
            int32_t imm = (int32_t)(((instr >> 31) & 1) << 12 | ((instr >> 7) & 1) << 11 | ((instr >> 25) & 0x3F) << 5 | ((instr >> 8) & 0xF) << 1);
            imm = sign_extend((uint32_t)imm, 13);
            int32_t v1 = (int32_t)cpu->regs[rs1];
            int32_t v2 = (int32_t)cpu->regs[rs2];
            uint32_t u1 = cpu->regs[rs1], u2 = cpu->regs[rs2];
            bool take = false;
            if (funct3 == 0) take = (u1 == u2);
            else if (funct3 == 1) take = (u1 != u2);
            else if (funct3 == 4) take = (v1 < v2);
            else if (funct3 == 5) take = (v1 >= v2);
            else if (funct3 == 6) take = (u1 < u2);
            else if (funct3 == 7) take = (u1 >= u2);
            if (take) cpu->pc = cur_pc + (uint32_t)imm;
            break;
        }
        case OP_LOAD: {
            int32_t imm = sign_extend(instr >> 20, 12);
            uint32_t addr = cpu->regs[rs1] + (uint32_t)imm;
            // MMIO reads (easy6502 compat): random byte / last key.
            // base+offset addressing, so match (base&~0xFFF)+offset too.
            if (addr == 0xFE || addr == 0x1FE) {
                if (funct3 == 2) cpu->regs[rd] = (uint32_t)(rand() & 0xFF);
                else if (funct3 == 0) { int8_t v = (int8_t)(rand() & 0xFF); cpu->regs[rd] = (uint32_t)(int32_t)v; }
                else cpu->regs[rd] = (uint32_t)(rand() & 0xFF);
                break;
            }
            if (addr == 0xFF || addr == 0x1FF) {
                if (funct3 == 2) cpu->regs[rd] = cpu->last_key;
                else if (funct3 == 0) { int8_t v = (int8_t)cpu->last_key; cpu->regs[rd] = (uint32_t)(int32_t)v; }
                else cpu->regs[rd] = cpu->last_key;
                break;
            }
            if (addr >= MEM_SIZE) break;
            if (funct3 == 2) { // lw
                if (addr + 3 < MEM_SIZE)
                    cpu->regs[rd] = (uint32_t)cpu->mem[addr] | (uint32_t)cpu->mem[addr + 1] << 8 | (uint32_t)cpu->mem[addr + 2] << 16 | (uint32_t)cpu->mem[addr + 3] << 24;
            } else if (funct3 == 0) { // lb
                int8_t v = (int8_t)cpu->mem[addr]; cpu->regs[rd] = (uint32_t)(int32_t)v;
            } else if (funct3 == 1) { // lh
                int16_t v = (int16_t)(cpu->mem[addr] | cpu->mem[addr + 1] << 8); cpu->regs[rd] = (uint32_t)(int32_t)v;
            } else if (funct3 == 4) { // lbu
                cpu->regs[rd] = cpu->mem[addr];
            } else if (funct3 == 5) {
                cpu->regs[rd] = (uint32_t)cpu->mem[addr] | (uint32_t)cpu->mem[addr + 1] << 8;
            }
            break;
        }
        case OP_STORE: {
            int32_t imm = (int32_t)(((instr >> 25) << 5) | ((instr >> 7) & 0x1F));
            imm = sign_extend((uint32_t)imm, 12);
            uint32_t addr = cpu->regs[rs1] + (uint32_t)imm;
            // MMIO 0x1000: print char to stdout
            if (addr == 0x1000) {
                fputc((int)(cpu->regs[rs2] & 0xFF), stdout); fflush(stdout);
                break;
            }
            if (addr >= MEM_SIZE) break;
            if (funct3 == 2) { // sw
                if (addr + 3 < MEM_SIZE) {
                    cpu->mem[addr]     = cpu->regs[rs2] & 0xFF;
                    cpu->mem[addr + 1] = (cpu->regs[rs2] >> 8) & 0xFF;
                    cpu->mem[addr + 2] = (cpu->regs[rs2] >> 16) & 0xFF;
                    cpu->mem[addr + 3] = (cpu->regs[rs2] >> 24) & 0xFF;
                }
            } else if (funct3 == 0) { // sb
                cpu->mem[addr] = cpu->regs[rs2] & 0xFF;
            } else if (funct3 == 1) { // sh
                cpu->mem[addr]     = cpu->regs[rs2] & 0xFF;
                cpu->mem[addr + 1] = (cpu->regs[rs2] >> 8) & 0xFF;
            }
            break;
        }
        case OP_IMM: {
            int32_t imm = sign_extend(instr >> 20, 12);
            uint32_t shamt = (instr >> 20) & 0x1F;
            if (funct3 == 0) {
                if (rd != 0) cpu->regs[rd] = cpu->regs[rs1] + (uint32_t)imm;
            } else if (funct3 == 1) {
                if (rd != 0) cpu->regs[rd] = cpu->regs[rs1] << shamt;
            } else if (funct3 == 5) {
                if (funct7 == 0) { if (rd != 0) cpu->regs[rd] = cpu->regs[rs1] >> shamt; }
                else { if (rd != 0) cpu->regs[rd] = (uint32_t)((int32_t)cpu->regs[rs1] >> (int)shamt); }
            } else if (funct3 == 2) { if (rd != 0) cpu->regs[rd] = ((int32_t)cpu->regs[rs1] < imm) ? 1 : 0; }
            else if (funct3 == 3) { if (rd != 0) cpu->regs[rd] = (cpu->regs[rs1] < (uint32_t)imm) ? 1 : 0; }
            else if (funct3 == 4) { if (rd != 0) cpu->regs[rd] = cpu->regs[rs1] ^ (uint32_t)imm; }
            else if (funct3 == 6) { if (rd != 0) cpu->regs[rd] = cpu->regs[rs1] | (uint32_t)imm; }
            else if (funct3 == 7) { if (rd != 0) cpu->regs[rd] = cpu->regs[rs1] & (uint32_t)imm; }
            break;
        }
        case OP_OP: {
            if (funct3 == 0) {
                if (funct7 == 0) { if (rd != 0) cpu->regs[rd] = cpu->regs[rs1] + cpu->regs[rs2]; }
                else { if (rd != 0) cpu->regs[rd] = cpu->regs[rs1] - cpu->regs[rs2]; }
            } else if (funct3 == 1) { if (rd != 0) cpu->regs[rd] = cpu->regs[rs1] << (cpu->regs[rs2] & 0x1F); }
            else if (funct3 == 2) { if (rd != 0) cpu->regs[rd] = ((int32_t)cpu->regs[rs1] < (int32_t)cpu->regs[rs2]) ? 1 : 0; }
            else if (funct3 == 3) { if (rd != 0) cpu->regs[rd] = (cpu->regs[rs1] < cpu->regs[rs2]) ? 1 : 0; }
            else if (funct3 == 4) { if (rd != 0) cpu->regs[rd] = cpu->regs[rs1] ^ cpu->regs[rs2]; }
            else if (funct3 == 5) {
                if (funct7 == 0) { if (rd != 0) cpu->regs[rd] = cpu->regs[rs1] >> (cpu->regs[rs2] & 0x1F); }
                else { if (rd != 0) cpu->regs[rd] = (uint32_t)((int32_t)cpu->regs[rs1] >> (int)(cpu->regs[rs2] & 0x1F)); }
            } else if (funct3 == 6) { if (rd != 0) cpu->regs[rd] = cpu->regs[rs1] | cpu->regs[rs2]; }
            else if (funct3 == 7) { if (rd != 0) cpu->regs[rd] = cpu->regs[rs1] & cpu->regs[rs2]; }
            break;
        }
        case OP_SYSTEM: {
            if (instr == 0x00000073) { // ecall
                if (cpu->regs[17] == 1) { // a7==1
                    fputc((int)(cpu->regs[10] & 0xFF), stdout); fflush(stdout);
                }
            } else if (instr == 0x10200073) { // sret - nop
            }
            break;
        }
        default: break;
    }

    cpu->regs[0] = 0;

    // halt on infinite loop j . (jal x0,0)
    if (cpu->pc == cur_pc) {
        cpu->halted = true;
        return false;
    }
    return true;
}
