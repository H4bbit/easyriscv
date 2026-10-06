# Solution: Fibonacci N=10 -> 55 (vector 0 1 1 2 3 5 8 13 21 34 55)
.section .text
.globl _start
_start:
    li a0, 10
    li s0, 0x000
    li t0, 0
    sw t0, 0(s0)
    beq a0, zero, done
    li t1, 1
    sw t1, 4(s0)
    li t2, 1
    beq a0, t2, ready
loop:
    add t3, t0, t1
    mv  t0, t1
    mv  t1, t3
    addi t2, t2, 1
    slli t4, t2, 2
    add  t5, s0, t4
    sw   t3, 0(t5)
    blt  t2, a0, loop
ready:
    mv a0, t1
done:
    j done
# Expected: a0=55, RAM 0x000 = 0 1 1 2 3 5 8 13 (first 8 words)
