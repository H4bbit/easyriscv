# Renamed: old 04-branching → new 04-loop (+ new 05-compares; see book/04-loop.md).
# Branching - counted loop with explicit compare branches
# Counts down from 8 to 3, writing pixel color at 0x200 each iteration
# Canonical compare first (bne on two registers), shorthand after
# (bnez against zero). No flags register anywhere.

.section .text
.globl _start
_start:
    li t0, 8          # counter = 8
    li t1, 0x200      # framebuffer base
    li t2, 3          # target = 3

loop:
    addi t0, t0, -1       # count down
    sb   t0, 0(t1)        # write color at 0x200 - 1 byte/pixel
    bne  t0, t2, loop     # canonical: branch on two registers

    sub  t3, t0, t2       # t3 = 3 - 3 = 0 (the loop really ended at 3)
    bnez t3, loop         # shorthand for bne t3, zero, loop: never taken

    # store final value at next pixel to observe halt
    sb   t0, 1(t1)
    j    .                # halt: shorthand for jal x0, .

# Exercises (see book/04-loop.md):
# 1. Change BNE to BEQ and observe fall-through vs loop
# 2. Use BLT/BGE to branch on signed less-than (e.g., blt t0, t2, loop)
# 3. Count up with addi + bne; then translate bnez <-> bne ..., zero
