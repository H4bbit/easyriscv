#pragma once
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

// Modern C23: constexpr instead of #define for typed constants
// Memory map (mirrors easy6502: code lives ABOVE the screen, state below):
//   0x000-0x0FF  RAM / zero-page (variables, game state)
//   0x100-0x1FF  stack (grows down from 0x1FC)
//   0x200-0x5FF  framebuffer 32x32, 1 byte/pixel (pixels ONLY)
//   0x600-...    code (PC starts at PROG_BASE)
//   0xFE/0xFF    MMIO random/last-key (intercepted on LOAD, not RAM)
//   0x1000       MMIO print-char port (intercepted on STORE)
constexpr size_t MEM_SIZE = 4096;
constexpr size_t FB_BASE  = 0x200;
constexpr size_t FB_SIZE  = 0x400; // 32*32
constexpr size_t PROG_BASE = 0x600;
constexpr size_t STACK_TOP = 0x1FC;

static_assert(FB_BASE + FB_SIZE <= MEM_SIZE, "framebuffer must fit in memory");
static_assert(PROG_BASE + FB_SIZE <= MEM_SIZE, "code area must fit in memory");

typedef struct {
    uint32_t regs[32];
    uint32_t pc;
    uint8_t  mem[MEM_SIZE];
    size_t   prog_size;
    bool     halted;
    // MMIO state kept OUT of mem[] so big programs (>=254 bytes) don't
    // get their code stomped: 0xFE reads fresh rand, 0xFF reads last_key.
    uint8_t  last_key; // ASCII of last key pressed (0 = none), set by UI
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
