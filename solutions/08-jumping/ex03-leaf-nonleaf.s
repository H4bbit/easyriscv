# Solution: leaf needs no save, non-leaf must save ra
# Canonical returns first (jalr x0, 0(ra)), alias spellings after
# (ret, jr): all three encode the same jalr. disasm prints ret/jr.
.section .text
.globl _start
_start:
    li sp, 0x1FC
    li t1, 0x200
    li t0, 10
    jal ra, double_add  # non-leaf: calls add_one twice
    sb t0, 0(t1)        # FB[0] = 12
    jal x0, .           # canonical halt: disasm prints j

add_one:                # leaf: calls nobody, ra untouched
    addi t0, t0, 1
    jalr x0, 0(ra)      # canonical return

double_add:             # non-leaf: must save ra before jal
    addi sp, sp, -4
    sw ra, 0(sp)
    jal ra, add_one     # +1 (would clobber ra without save)
    jal ra, add_one     # +1
    lw ra, 0(sp)
    addi sp, sp, 4
    ret                 # shorthand twin: jalr x0, 0(x1)
# Expected: FB[0]=12 (10+1+1), sp back at 0x1FC
