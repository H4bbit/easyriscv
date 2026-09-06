#pragma once
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

// Modern C23: constexpr instead of #define for typed constants
constexpr size_t MEM_SIZE = 4096;
constexpr size_t FB_BASE  = 0x200;
constexpr size_t FB_SIZE  = 0x400; // 32*32

static_assert(FB_BASE + FB_SIZE <= MEM_SIZE, "framebuffer must fit in memory");

typedef struct {
    uint32_t regs[32];
    uint32_t pc;
    uint8_t  mem[MEM_SIZE];
    size_t   prog_size;
    bool     halted;
} CPU;

// Shared tables - extern to avoid per-TU copies (modern: single definition in vm.c)
extern const char *const reg_names[32];
extern const int palette_256[16];

// Opcodes as typed enum (self-documenting, replaces magic 0x37 etc.)
enum : uint8_t {
    OP_LUI    = 0x37,
    OP_AUIPC  = 0x17,
    OP_JAL    = 0x6F,
    OP_JALR   = 0x67,
    OP_BRANCH = 0x63,
    OP_LOAD   = 0x03,
    OP_STORE  = 0x23,
    OP_IMM    = 0x13,
    OP_OP     = 0x33,
    OP_SYSTEM = 0x73,
};

[[nodiscard]] bool cpu_load_bin(CPU *cpu, const char *path);
void cpu_reset(CPU *cpu);
[[nodiscard]] bool cpu_step(CPU *cpu);
const char *decode_to_str(uint32_t instr, uint32_t pc, char *out, size_t outlen);
[[nodiscard]] uint32_t cpu_fetch(const CPU *cpu);
