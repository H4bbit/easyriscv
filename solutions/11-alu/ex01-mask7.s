# Solution: mask 0x07 -> range 0-7, +2 -> 2-9
.section .text
.globl _start
_start:
    li t0, 0xFE
    lbu t1, 0(t0)      # random 0-255
    andi t1, t1, 0x07  # keep low 3 bits -> 0-7
    addi t1, t1, 2     # 2-9
    li t2, 0x200
    sb t1, 0(t2)
    j .
# Expected: FB[0] = 2-9 (random each run)
