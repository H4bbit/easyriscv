# Solution: beq instead of bne - falls through after one iteration
# Canonical beq on two registers. For the translation exercise, the
# shorthand twin is `beqz t0, loop` only when one side is zero — here
# both sides are registers, so there is no beqz spelling of this test.
# t0 goes 8->7; 7 != 3 so beq does NOT branch, execution falls to the
# final store and halts with t0=7, FB[0]=FB[1]=7.
# Contrast with bne, which loops until t0==3.
.section .text
.globl _start
_start:
    li t0, 8
    li t1, 0x200
    li t2, 3
loop:
    addi t0, t0, -1
    sb t0, 0(t1)
    beq t0, t2, loop   # branches only when equal: not taken here, falls through
    sb t0, 1(t1)
    j .
# Expected: halt with t0=7, FB[0]=FB[1]=7 (vs 3 3 with bne)
# Translation twin (same encoding): no beqz form exists for two registers;
# beqz appears only as beq rs, zero, label — see ex03-count-up for that pair.
