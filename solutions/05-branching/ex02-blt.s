// Solution: blt instead of bne - exits after one iteration
// t0 goes 8->7; 7<3 (signed) is false, so the loop exits at once.
// Contrast with bne, which loops until t0==3.
.section .text
.globl _start
_start:
    li t0, 8
    li t1, 0x200
    li t2, 3
loop:
    addi t0, t0, -1
    sb t0, 0(t1)
    blt t0, t2, loop   // taken only while t0 < 3: never here
    sb t0, 1(t1)
    j .
// Expected: halt with t0=7, FB[0]=FB[1]=7 (vs 3 3 with bne)
