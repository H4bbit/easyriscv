.section .text
.globl _start
_start:
    li a0, 7          // N = 7 (compute 7th Fibonacci number)
    li t0, 0          // F(0) = 0
    beq a0, zero, done
    li t1, 1          // F(1) = 1
    li t2, 1          // counter = 1
    beq a0, t2, ready
loop:
    add t3, t0, t1    // t3 = F(n-1) + F(n-2)
    mv  t0, t1
    mv  t1, t3
    addi t2, t2, 1    // counter++
    blt  t2, a0, loop
ready:
    mv a0, t1         // a0 = result (13 for N=7)
done:
    j done
