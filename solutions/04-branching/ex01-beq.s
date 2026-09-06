// Solution: beq loops only if equal - here it never loops
.section .text
.globl _start
_start:
    li t0, 8
    li t1, 0x200
    li t2, 3
loop:
    addi t0, t0, -1
    sb t0, 0(t1)
    beq t0, t2, loop   // only branch when equal, so loops once at t0=3 then halts? Actually will loop forever when t0 hits 3
    sb t0, 1(t1)
    j .
