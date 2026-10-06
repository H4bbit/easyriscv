# Solution: beq instead of bne - falls through after one iteration
# t0 goes 8->7; 7 != 3 so beq does NOT branch, execution falls to the
# final store and halts with t0=7, FB[0]=FB[1]=7.
# Contrast with bne, which loops until t0==3.
.section .text
.globl _start
_start:
    li t0, 8
    li t1, 0x200
    li t2, 3
loop:
    addi t0, t0, -1
    sb t0, 0(t1)
    beq t0, t2, loop   # branches only when equal: not taken here, falls through
    sb t0, 1(t1)
    j .
