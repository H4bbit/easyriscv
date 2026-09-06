// Memory - RISC-V load/store vs 6502 addressing modes
// Demonstrates lw/sw, offsets, and base+offset (only modes in RISC-V)
// Framebuffer at 0x200 is just memory - same as easy6502 $0200-$05FF

.section .text
.globl _start
_start:
    li t0, 0x300      // RAM base (like zero page)
    li t1, 42
    sw t1, 0(t0)      // absolute: sw t1, 0(t0)  -> *0x300 = 42
    li t1, 99
    sw t1, 4(t0)      // absolute+4: sw t1, 4(t0) -> *0x304 = 99

    // indexed via register
    li t2, 8          // offset = 8
    li t1, 77
    add t3, t0, t2    // t3 = base + offset
    sw t1, 0(t3)      // store at 0x308

    // load back and copy to framebuffer
    lw t4, 0(t0)      // t4 = *0x300 (42)
    lw t5, 4(t0)      // t5 = *0x304 (99)
    add t4, t4, t5    // t4 = 141
    li s0, 0x200      // FB base (keep separate reg)
    sb t4, 0(s0)      // FB[0] = 141 & 0xF = 13 (light green) - 1 byte/pixel

    // byte store/load
    li t1, 0xAB
    sb t1, 12(t0)     // byte at 0x30C
    lbu s1, 12(t0)    // load byte unsigned -> s1=171
    sb s1, 1(s0)      // FB[1] = 171 & 0xF = 11 (dark grey)

    j .

// Exercises:
// 1. Try lh/lhu vs lw - what happens with alignment?
// 2. Use slli to compute index*4 (like fib-ram) for array access
// 3. Copy 0x300 vector to framebuffer 0x200 in a loop
