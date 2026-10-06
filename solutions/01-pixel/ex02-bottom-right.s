# Solution: draw white pixel at bottom-right (31,31) -> offset 1023
.section .text
.globl _start
_start:
    li t0, 1
    li t1, 0x200
    sb t0, 1023(t1)   # 32*32-1
    j .
