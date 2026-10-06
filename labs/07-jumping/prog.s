# Jumping - jal/jalr subroutine calls and unconditional jumps
# jal rd stores pc+4 in rd and jumps: it never touches the stack.
# Saving ra with sw is software convention: your code pushes, your code
# pops. A nested jal overwrites ra, so a function that calls another
# must save ra first (see outer below; solutions ex01 shows the breakage).
# First half spells everything canonically (explicit x0/x1), second half
# uses the aliases (j, ret, jr) for the same observable FB = 1, 5, 15.

.section .text
.globl _start
_start:
    li t0, 0
    jal x1, inc_one   # canonical call: x1 is ra, no stack involved
    # t0 should be 1 here
    li t1, 0x200
    sb t0, 0(t1)       # FB[0] = 1 (1 byte/pixel)

    jal x0, after     # canonical jump: x0 discards the return address
    li t0, 99         # skipped
after:
    addi t0, t0, 4    # t0 = 5
    sb t0, 1(t1)      # FB[1] = 5

    # nested call test: save ra on the stack before the inner call
    li sp, 0x1FC
    jal ra, outer     # jal ra is jal x1: same canonical instruction
    sb t0, 2(t1)      # FB[2] should be 15
    j .               # halt: shorthand for jal x0, .

inc_one:
    addi t0, t0, 1
    jalr x0, 0(x1)    # canonical return through x1 (= ra)

outer:
    addi sp, sp, -4
    sw ra, 0(sp)      # inner would overwrite ra (see solutions ex01)
    addi t0, t0, 1    # t0 5->6
    jal ra, inner
    lw ra, 0(sp)
    addi sp, sp, 4
    ret               # shorthand for jalr x0, 0(x1)

inner:
    addi t0, t0, 9    # t0 6->15
    jr ra             # shorthand for jalr x0, 0(ra)

# Expected FB: 1, 5, 15
