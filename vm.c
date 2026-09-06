#include "vm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

bool cpu_load_bin(CPU *cpu, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fseek(f,0,SEEK_END);
    long sz = ftell(f);
    fseek(f,0,SEEK_SET);
    if (sz > (long)MEM_SIZE) sz = MEM_SIZE;
    if (sz < 0) sz = 0;
    size_t n = fread(cpu->mem,1,sz,f);
    fclose(f);
    cpu->prog_size = n;
    // zera resto
    if (n < MEM_SIZE) memset(cpu->mem+n,0,MEM_SIZE-n);
    cpu->pc = 0;
    memset(cpu->regs,0,sizeof(cpu->regs));
    cpu->halted = false;
    return true;
}

void cpu_reset(CPU *cpu) {
    cpu->pc = 0;
    memset(cpu->regs,0,sizeof(cpu->regs));
    cpu->halted = false;
}

uint32_t cpu_fetch(CPU *cpu) {
    if (cpu->pc+3 >= MEM_SIZE) return 0;
    return cpu->mem[cpu->pc] | (cpu->mem[cpu->pc+1]<<8) | (cpu->mem[cpu->pc+2]<<16) | (cpu->mem[cpu->pc+3]<<24);
}

static inline int32_t sign_extend(uint32_t val, int bits){
    int32_t m = 1u << (bits-1);
    return (int32_t)((val ^ m) - m);
}

const char *decode_to_str(uint32_t instr, uint32_t pc, char *out, size_t outlen){
    uint32_t opcode = instr & 0x7F;
    uint32_t rd = (instr>>7)&0x1F;
    uint32_t funct3 = (instr>>12)&0x7;
    uint32_t rs1 = (instr>>15)&0x1F;
    uint32_t rs2 = (instr>>20)&0x1F;
    uint32_t funct7 = (instr>>25)&0x7F;

    if (instr==0) { snprintf(out,outlen,"unimp"); return out; }

    if (opcode==0x37){ // LUI
        int32_t imm = instr & 0xFFFFF000;
        snprintf(out,outlen,"lui     %s,0x%x",reg_names[rd], imm>>12);
        return out;
    }
    if (opcode==0x17){ // AUIPC
        int32_t imm = instr & 0xFFFFF000;
        snprintf(out,outlen,"auipc   %s,0x%x",reg_names[rd], imm>>12);
        return out;
    }
    if (opcode==0x6F){ // JAL
        int32_t imm = ((instr>>31)&1)<<20 | ((instr>>12)&0xFF)<<12 | ((instr>>20)&1)<<11 | ((instr>>21)&0x3FF)<<1;
        imm = sign_extend(imm,21);
        snprintf(out,outlen,"jal     %s,0x%x",reg_names[rd], pc+imm);
        return out;
    }
    if (opcode==0x67){ // JALR
        int32_t imm = sign_extend(instr>>20,12);
        snprintf(out,outlen,"jalr    %s,%d(%s)",reg_names[rd], imm, reg_names[rs1]);
        return out;
    }
    if (opcode==0x63){ // BRANCH
        int32_t imm = ((instr>>31)&1)<<12 | ((instr>>7)&1)<<11 | ((instr>>25)&0x3F)<<5 | ((instr>>8)&0xF)<<1;
        imm = sign_extend(imm,13);
        const char *mn="b??";
        if(funct3==0) mn="beq";
        else if(funct3==1) mn="bne";
        else if(funct3==4) mn="blt";
        else if(funct3==5) mn="bge";
        else if(funct3==6) mn="bltu";
        else if(funct3==7) mn="bgeu";
        if(funct3==0 && rs2==0) snprintf(out,outlen,"beqz    %s,0x%x",reg_names[rs1], pc+imm);
        else if(funct3==1 && rs2==0) snprintf(out,outlen,"bnez    %s,0x%x",reg_names[rs1], pc+imm);
        else snprintf(out,outlen,"%-7s %s,%s,0x%x",mn,reg_names[rs1],reg_names[rs2], pc+imm);
        return out;
    }
    if (opcode==0x03){ // LOAD
        int32_t imm = sign_extend(instr>>20,12);
        const char *mn="lb";
        if(funct3==0) mn="lb"; else if(funct3==1) mn="lh"; else if(funct3==2) mn="lw";
        else if(funct3==4) mn="lbu"; else if(funct3==5) mn="lhu";
        snprintf(out,outlen,"%-7s %s,%d(%s)",mn,reg_names[rd],imm,reg_names[rs1]);
        return out;
    }
    if (opcode==0x23){ // STORE
        int32_t imm = ((instr>>25)<<5) | ((instr>>7)&0x1F);
        imm = sign_extend(imm,12);
        const char *mn="sb";
        if(funct3==0) mn="sb"; else if(funct3==1) mn="sh"; else if(funct3==2) mn="sw";
        snprintf(out,outlen,"%-7s %s,%d(%s)",mn,reg_names[rs2],imm,reg_names[rs1]);
        return out;
    }
    if (opcode==0x13){ // OP-IMM
        int32_t imm = sign_extend(instr>>20,12);
        uint32_t shamt = (instr>>20)&0x3F;
        if(funct3==0){
            if(rs1==0) snprintf(out,outlen,"li      %s,%d",reg_names[rd],imm);
            else if(imm==0) snprintf(out,outlen,"mv      %s,%s",reg_names[rd],reg_names[rs1]);
            else snprintf(out,outlen,"addi    %s,%s,%d",reg_names[rd],reg_names[rs1],imm);
        } else if(funct3==1 && funct7==0) snprintf(out,outlen,"slli    %s,%s,%u",reg_names[rd],reg_names[rs1],shamt&0x1F);
        else if(funct3==5){
            if(funct7==0) snprintf(out,outlen,"srli    %s,%s,%u",reg_names[rd],reg_names[rs1],shamt&0x1F);
            else if(funct7==0x20) snprintf(out,outlen,"srai    %s,%s,%u",reg_names[rd],reg_names[rs1],shamt&0x1F);
            else snprintf(out,outlen,"op-imm  0x%08x",instr);
        } else {
            const char *mn="op-imm";
            if(funct3==2) mn="slti"; else if(funct3==3) mn="sltiu"; else if(funct3==4) mn="xori"; else if(funct3==6) mn="ori"; else if(funct3==7) mn="andi";
            snprintf(out,outlen,"%-7s %s,%s,%d",mn,reg_names[rd],reg_names[rs1],imm);
        }
        return out;
    }
    if (opcode==0x33){ // OP
        const char *mn="add";
        if(funct3==0 && funct7==0) mn="add";
        else if(funct3==0 && funct7==0x20) mn="sub";
        else if(funct3==1) mn="sll";
        else if(funct3==2) mn="slt";
        else if(funct3==3) mn="sltu";
        else if(funct3==4) mn="xor";
        else if(funct3==5 && funct7==0) mn="srl";
        else if(funct3==5 && funct7==0x20) mn="sra";
        else if(funct3==6) mn="or";
        else if(funct3==7) mn="and";
        else mn="op";
        snprintf(out,outlen,"%-7s %s,%s,%s",mn,reg_names[rd],reg_names[rs1],reg_names[rs2]);
        return out;
    }
    if (opcode==0x73){
        if(instr==0x00000073) snprintf(out,outlen,"ecall");
        else if(instr==0x10200073) snprintf(out,outlen,"sret");
        else snprintf(out,outlen,"system  0x%08x",instr);
        return out;
    }
    snprintf(out,outlen,"unknown 0x%08x",instr);
    return out;
}

bool cpu_step(CPU *cpu){
    if(cpu->halted) return false;
    cpu->regs[0]=0;
    if(cpu->pc+3 >= MEM_SIZE){ cpu->halted=true; return false; }
    uint32_t instr = cpu_fetch(cpu);
    uint32_t cur_pc = cpu->pc;
    cpu->pc += 4;
    uint32_t opcode = instr & 0x7F;
    uint32_t rd = (instr>>7)&0x1F;
    uint32_t funct3 = (instr>>12)&0x7;
    uint32_t rs1 = (instr>>15)&0x1F;
    uint32_t rs2 = (instr>>20)&0x1F;
    uint32_t funct7 = (instr>>25)&0x7F;

    switch(opcode){
        case 0x37: { // LUI
            int32_t imm = instr & 0xFFFFF000;
            if(rd) cpu->regs[rd] = imm;
            break;
        }
        case 0x17: { // AUIPC
            int32_t imm = instr & 0xFFFFF000;
            if(rd) cpu->regs[rd] = cur_pc + imm;
            break;
        }
        case 0x6F: { // JAL
            int32_t imm = ((instr>>31)&1)<<20 | ((instr>>12)&0xFF)<<12 | ((instr>>20)&1)<<11 | ((instr>>21)&0x3FF)<<1;
            imm = sign_extend(imm,21);
            if(rd) cpu->regs[rd] = cur_pc+4;
            cpu->pc = cur_pc + imm;
            break;
        }
        case 0x67: { // JALR
            int32_t imm = sign_extend(instr>>20,12);
            uint32_t ret = cur_pc+4;
            cpu->pc = (cpu->regs[rs1] + imm) & ~1u;
            if(rd) cpu->regs[rd]=ret;
            break;
        }
        case 0x63: { // BRANCH
            int32_t imm = ((instr>>31)&1)<<12 | ((instr>>7)&1)<<11 | ((instr>>25)&0x3F)<<5 | ((instr>>8)&0xF)<<1;
            imm = sign_extend(imm,13);
            int32_t v1 = (int32_t)cpu->regs[rs1];
            int32_t v2 = (int32_t)cpu->regs[rs2];
            uint32_t u1 = cpu->regs[rs1], u2 = cpu->regs[rs2];
            bool take=false;
            if(funct3==0) take=(u1==u2);
            else if(funct3==1) take=(u1!=u2);
            else if(funct3==4) take=(v1 < v2);
            else if(funct3==5) take=(v1 >= v2);
            else if(funct3==6) take=(u1 < u2);
            else if(funct3==7) take=(u1 >= u2);
            if(take) cpu->pc = cur_pc + imm;
            break;
        }
        case 0x03: { // LOAD
            int32_t imm = sign_extend(instr>>20,12);
            uint32_t addr = cpu->regs[rs1] + imm;
            if(addr >= MEM_SIZE) break;
            if(funct3==2){ // lw
                if(addr+3 < MEM_SIZE)
                    cpu->regs[rd] = cpu->mem[addr] | (cpu->mem[addr+1]<<8) | (cpu->mem[addr+2]<<16) | (cpu->mem[addr+3]<<24);
            } else if(funct3==0){ // lb
                int8_t v = (int8_t)cpu->mem[addr]; cpu->regs[rd]=(int32_t)v;
            } else if(funct3==1){ // lh
                int16_t v = cpu->mem[addr] | (cpu->mem[addr+1]<<8); cpu->regs[rd]=(int32_t)(int16_t)v;
            } else if(funct3==4){ // lbu
                cpu->regs[rd]=cpu->mem[addr];
            } else if(funct3==5){
                cpu->regs[rd]= cpu->mem[addr] | (cpu->mem[addr+1]<<8);
            }
            break;
        }
        case 0x23: { // STORE
            int32_t imm = ((instr>>25)<<5) | ((instr>>7)&0x1F);
            imm = sign_extend(imm,12);
            uint32_t addr = cpu->regs[rs1] + imm;
            // MMIO 0x1000: print char (kernel.s)
            if(addr==0x1000){
                fputc(cpu->regs[rs2] & 0xFF, stdout); fflush(stdout);
                break;
            }
            if(addr >= MEM_SIZE) break;
            if(funct3==2){ // sw
                if(addr+3 < MEM_SIZE){
                    cpu->mem[addr]= cpu->regs[rs2] & 0xFF;
                    cpu->mem[addr+1]=(cpu->regs[rs2]>>8)&0xFF;
                    cpu->mem[addr+2]=(cpu->regs[rs2]>>16)&0xFF;
                    cpu->mem[addr+3]=(cpu->regs[rs2]>>24)&0xFF;
                }
            } else if(funct3==0){ // sb
                cpu->mem[addr]= cpu->regs[rs2] & 0xFF;
            } else if(funct3==1){ // sh
                cpu->mem[addr]= cpu->regs[rs2] & 0xFF;
                cpu->mem[addr+1]=(cpu->regs[rs2]>>8)&0xFF;
            }
            break;
        }
        case 0x13: { // OP-IMM
            int32_t imm = sign_extend(instr>>20,12);
            uint32_t shamt = (instr>>20)&0x1F;
            if(funct3==0){
                if(rd) cpu->regs[rd] = cpu->regs[rs1] + imm;
            } else if(funct3==1){
                if(rd) cpu->regs[rd] = cpu->regs[rs1] << shamt;
            } else if(funct3==5){
                if(funct7==0){ if(rd) cpu->regs[rd] = cpu->regs[rs1] >> shamt; }
                else { if(rd) cpu->regs[rd] = ((int32_t)cpu->regs[rs1]) >> shamt; }
            } else if(funct3==2){ if(rd) cpu->regs[rd]= ((int32_t)cpu->regs[rs1] < imm)?1:0; }
            else if(funct3==3){ if(rd) cpu->regs[rd]= (cpu->regs[rs1] < (uint32_t)imm)?1:0; }
            else if(funct3==4){ if(rd) cpu->regs[rd]= cpu->regs[rs1] ^ imm; }
            else if(funct3==6){ if(rd) cpu->regs[rd]= cpu->regs[rs1] | imm; }
            else if(funct3==7){ if(rd) cpu->regs[rd]= cpu->regs[rs1] & imm; }
            break;
        }
        case 0x33: { // OP
            if(funct3==0){
                if(funct7==0){ if(rd) cpu->regs[rd]= cpu->regs[rs1]+cpu->regs[rs2]; }
                else { if(rd) cpu->regs[rd]= cpu->regs[rs1]-cpu->regs[rs2]; }
            } else if(funct3==1){ if(rd) cpu->regs[rd]= cpu->regs[rs1] << (cpu->regs[rs2]&0x1F); }
            else if(funct3==2){ if(rd) cpu->regs[rd]= ((int32_t)cpu->regs[rs1] < (int32_t)cpu->regs[rs2])?1:0; }
            else if(funct3==3){ if(rd) cpu->regs[rd]= (cpu->regs[rs1] < cpu->regs[rs2])?1:0; }
            else if(funct3==4){ if(rd) cpu->regs[rd]= cpu->regs[rs1]^cpu->regs[rs2]; }
            else if(funct3==5){
                if(funct7==0){ if(rd) cpu->regs[rd]= cpu->regs[rs1] >> (cpu->regs[rs2]&0x1F); }
                else { if(rd) cpu->regs[rd]= ((int32_t)cpu->regs[rs1]) >> (cpu->regs[rs2]&0x1F); }
            } else if(funct3==6){ if(rd) cpu->regs[rd]= cpu->regs[rs1] | cpu->regs[rs2]; }
            else if(funct3==7){ if(rd) cpu->regs[rd]= cpu->regs[rs1] & cpu->regs[rs2]; }
            break;
        }
        case 0x73: { // SYSTEM
            if(instr==0x00000073){ // ecall - para demo, trata a7==1 como print char (compat kernel)
                if(cpu->regs[17]==1){ // a7==1
                    fputc(cpu->regs[10]&0xFF, stdout); fflush(stdout);
                }
            } else if(instr==0x10200073){ // sret - nop
            }
            break;
        }
        default: break;
    }

    cpu->regs[0]=0;

    // detecta loop infinito j . (jal x0,0) como halt
    if(cpu->pc==cur_pc){
        cpu->halted=true;
        return false;
    }
    return true;
}
