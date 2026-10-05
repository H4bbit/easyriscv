// Solution: store the low byte of each term (sb instead of sw)
// Bytes: same low 8 bits, so 0 1 1 2 3 5 8 13 (all terms < 256 here)
.section .text
.globl _start
_start:
    li a0, 7
    li s0, 0x000
    li t0, 0
    sb t0, 0(s0)
    beq a0, zero, done
    li t1, 1
    sb t1, 1(s0)
    li t2, 1
    beq a0, t2, ready
loop:
    add t3, t0, t1
    mv  t0, t1
    mv  t1, t3
    addi t2, t2, 1
    add  t5, s0, t2    // byte index: no scaling
    sb   t3, 0(t5)
    blt  t2, a0, loop
ready:
    mv a0, t1
done:
    j done
// Expected: a0=13, RAM bytes 0x00..0x07 = 0 1 1 2 3 5 8 13
