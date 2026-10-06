# Solution: count up from 0 to 5 with addi + bne, both spellings
# Canonical exit test first (bne on two registers), zero-shorthand after.
# t0 ends at 5, FB[0]=FB[1]=5. disasm answer key: `bne t0, t2` stays bne,
# `bne t3, zero, ...` folds to `bnez t3`.
.section .text
.globl _start
_start:
    li t0, 0
    li t1, 0x200
    li t2, 5
loop:
    addi t0, t0, 1
    sb t0, 0(t1)       # draw current count
    bne t0, t2, loop   # canonical: until t0 == 5
    sub t3, t0, t2     # 0: count really reached 5
    bne t3, zero, loop # canonical spelling of the zero test...
    bnez t3, loop      # ...shorthand twin: never taken, same encoding
    sb t0, 1(t1)
    j .
# Expected: halt with t0=5, FB[0]=FB[1]=5
