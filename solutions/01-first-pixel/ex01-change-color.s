# Solution: same three pixels in different colors
.section .text
.globl _start
_start:
    li t0, 2          # red (was white)
    li t1, 0x200
    sb t0, 0(t1)      # pixel (0,0)
    li t0, 0xE        # light blue (was green)
    sb t0, 1(t1)      # pixel (1,0)
    li t0, 7          # yellow (was orange)
    sb t0, 2(t1)      # pixel (2,0)
    j .
# Expected: FB[0..2] = 2 14 7
