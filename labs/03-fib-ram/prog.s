.section .text
.globl _start
_start:
    li a0, 7
    li s0, 0x000      # zero-page RAM base (code lives at 0x600, far away)
    li t0, 0
    sw t0, 0(s0)      # RAM[0x00] = 0
    beq a0, zero, done
    li t1, 1
    sw t1, 4(s0)      # RAM[0x004] = 1
    li t2, 1
    beq a0, t2, ready
loop:
    add t3, t0, t1
    mv  t0, t1
    mv  t1, t3
    addi t2, t2, 1
    slli t4, t2, 2
    add  t5, s0, t4
    sw   t3, 0(t5)    # RAM[s0 + t2*4] = t3
    blt  t2, a0, loop
ready:
    mv a0, t1
done:
    j done
