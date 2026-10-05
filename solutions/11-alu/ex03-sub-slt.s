// Solution: sub and slt both directions (10 vs 3)
.section .text
.globl _start
_start:
    li t3, 10
    li t4, 3
    sub t5, t3, t4     // 10 - 3 = 7
    slt t6, t3, t4     // 10 < 3 ? 0
    slt s0, t4, t3     // 3 < 10 ? 1
    li t1, 0x200
    sb t5, 0(t1)       // FB[0] = 7
    sb t6, 1(t1)       // FB[1] = 0
    sb s0, 2(t1)       // FB[2] = 1
    j .
// Expected: FB = 7 0 1
