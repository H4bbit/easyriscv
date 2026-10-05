// Branching - counted loop with explicit compare branches
// Counts down from 8 to 3, writing pixel color at 0x200 each iteration
// Demonstrates beq/bne with no flags register (explicit compare)

.section .text
.globl _start
_start:
    li t0, 8          // counter = 8 (like LDX #$08)
    li t1, 0x200      // framebuffer base
    li t2, 3          // target = 3 (like CPX #$03)

loop:
    addi t0, t0, -1       // DEX
    sb   t0, 0(t1)        // write color at 0x200 - 1 byte/pixel
    bne  t0, t2, loop     // branch if !=

    // store final value at next pixel to observe halt
    sb   t0, 1(t1)
    j    .                // halt

// Exercises:
// 1. Change BNE to BEQ and observe infinite loop vs immediate halt
// 2. Use BLT/BGE to branch on signed less-than (e.g., blt t0, t2, loop)
// 3. Try BLTU for unsigned compare
