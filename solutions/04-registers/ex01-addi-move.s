// Solution: move t0 to a0 without mv (addi with zero offset)
.section .text
.globl _start
_start:
    li t0, 42
    addi a0, t0, 0    // a0 = t0 + 0 (mv is a pseudo-op for this)
    j .
// Expected: halt with a0=42
