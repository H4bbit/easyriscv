// Solution: build the vector, then sum words 0..7 into a1 (0+1+1+2+3+5+8+13=33)
.section .text
.globl _start
_start:
    li a0, 7
    li s0, 0x000
    li t0, 0
    sw t0, 0(s0)
    beq a0, zero, sum
    li t1, 1
    sw t1, 4(s0)
    li t2, 1
    beq a0, t2, sum
loop:
    add t3, t0, t1
    mv  t0, t1
    mv  t1, t3
    addi t2, t2, 1
    slli t4, t2, 2
    add  t5, s0, t4
    sw   t3, 0(t5)
    blt  t2, a0, loop
sum:
    li a1, 0
    li t2, 0
addloop:
    slli t4, t2, 2
    add t5, s0, t4
    lw t3, 0(t5)
    add a1, a1, t3
    addi t2, t2, 1
    li t6, 8
    blt t2, t6, addloop
    mv a0, t1
    j sum_done
sum_done:
    j sum_done
// Expected: a0=13, a1=33
