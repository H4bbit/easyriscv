// Solution: slli for index*4
.section .text
.globl _start
_start:
    li t0, 0x000
    li t2, 3          // index 3
    slli t4, t2, 2    // *4
    add t3, t0, t4
    li t1, 99
    sw t1, 0(t3)      // store at 0x00C
    j .
