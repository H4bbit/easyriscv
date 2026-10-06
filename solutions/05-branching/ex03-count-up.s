# Solution: count up from 0 to 5 with addi + bne
.section .text
.globl _start
_start:
    li t0, 0
    li t1, 0x200
    li t2, 5
loop:
    addi t0, t0, 1
    sb t0, 0(t1)       # draw current count
    bne t0, t2, loop   # until t0 == 5
    sb t0, 1(t1)
    j .
# Expected: halt with t0=5, FB[0]=FB[1]=5
