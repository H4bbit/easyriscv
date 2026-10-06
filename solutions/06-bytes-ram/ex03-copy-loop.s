# Solution: copy 4 words from 0x000 to framebuffer 0x200
.section .text
.globl _start
_start:
    li t0, 0x000
    li t1, 1
    sw t1, 0(t0)
    li t1, 2
    sw t1, 4(t0)
    li t1, 3
    sw t1, 8(t0)
    li t1, 4
    sw t1, 12(t0)     # RAM[0x000..0x00C] = 1 2 3 4
    li t1, 0x200
    li t2, 0          # i = 0
copy:
    slli t3, t2, 2    # i * 4
    add t4, t0, t3
    lw t5, 0(t4)      # RAM[0x000 + i*4]
    add t4, t1, t3
    sw t5, 0(t4)      # FB[i*4] = value
    addi t2, t2, 1
    li t6, 4
    blt t2, t6, copy
    j .
# Expected: FB words = 1 2 3 4 (bytes: 1 0 0 0 2 0 0 0 ...)
