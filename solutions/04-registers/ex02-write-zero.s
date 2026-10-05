// Solution: writes to zero are ignored (x0 is hardwired to 0)
.section .text
.globl _start
_start:
    li t0, 42
    addi zero, t0, 0  // ignored: zero stays 0
    mv a0, zero       // a0 = 0, not 42
    j .
// Expected: halt with a0=0
