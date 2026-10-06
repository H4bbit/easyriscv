# Solution: print digit '5' via a 0x1000 store
.section .text
.globl _start
_start:
    li t0, 0x1000
    li t1, 53          # '5'
    sw t1, 0(t0)
    j .
# Expected: stdout shows '5'
