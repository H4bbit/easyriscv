#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MEM_SIZE 4096
#define FB_BASE  0x200
#define FB_SIZE  0x400  // 32*32

typedef struct {
    uint32_t regs[32];
    uint32_t pc;
    uint8_t  mem[MEM_SIZE];
    size_t   prog_size; // tamanho do binário carregado
    bool     halted;
} CPU;

static const char *reg_names[32] = {
    "zero","ra","sp","gp","tp","t0","t1","t2",
    "s0","s1","a0","a1","a2","a3","a4","a5","a6","a7",
    "s2","s3","s4","s5","s6","s7","s8","s9","s10","s11",
    "t3","t4","t5","t6"
};

// palette 16 cores do easy6502 -> mapeada para pares ncurses
static const int palette_256[16] = {
    16, 15,  88, 159,  127,  34,  19, 227,
    208, 94, 210, 242, 245, 120,  27, 250
};

bool cpu_load_bin(CPU *cpu, const char *path);
void cpu_reset(CPU *cpu);
bool cpu_step(CPU *cpu);
const char *decode_to_str(uint32_t instr, uint32_t pc, char *out, size_t outlen);
uint32_t cpu_fetch(CPU *cpu);
