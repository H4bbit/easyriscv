// Solution: high nibble via srli by 4 (0xAB -> 0x0A)
.section .text
.globl _start
_start:
    li t0, 0xFE
    lbu t1, 0(t0)      // random 0-255
    srli t1, t1, 4     // high nibble -> 0-15
    li t2, 0x200
    sb t1, 0(t2)
    j .
// Expected: FB[0] = 0-15 (random each run)
