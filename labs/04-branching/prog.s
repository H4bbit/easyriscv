// Branching - RISC-V equivalent of easy6502 BNE/BEQ lesson
// Counts down from 8 to 3, writing pixel color at 0x200 each iteration
// Demonstrates beq/bne vs 6502 BNE/BEQ (no flags, explicit compare)

.section .text
.globl _start
_start:
    li t0, 8          // counter = 8 (like LDX #$08)
    li t1, 0x200      // framebuffer base
    li t2, 3          // target = 3 (like CPX #$03)

loop:
    addi t0, t0, -1       // DEX
    sw   t0, 0(t1)        // STX $0200 - write color
    bne  t0, t2, loop     // BNE decrement - explicit rs1!=rs2

    // store final value at next pixel to observe halt
    sw   t0, 4(t1)
    j    .                // halt

// Exercises:
// 1. Change BNE to BEQ and observe infinite loop vs immediate halt
// 2. Use BLT/BGE to branch on signed less-than (e.g., blt t0, t2, loop)
// 3. Try BLTU for unsigned compare
