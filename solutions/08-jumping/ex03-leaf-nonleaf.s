// Solution: leaf needs no save, non-leaf must save ra
.section .text
.globl _start
_start:
    li sp, 0x900
    li t1, 0x200
    li t0, 10
    jal ra, double_add  // non-leaf: calls add_one twice
    sb t0, 0(t1)        // FB[0] = 12
    j .

add_one:                // leaf: calls nobody, ra untouched
    addi t0, t0, 1
    jalr zero, 0(ra)

double_add:             // non-leaf: must save ra before jal
    addi sp, sp, -4
    sw ra, 0(sp)
    jal ra, add_one     // +1 (would clobber ra without save)
    jal ra, add_one     // +1
    lw ra, 0(sp)
    addi sp, sp, 4
    jalr zero, 0(ra)
// Expected: FB[0]=12 (10+1+1), sp back at 0x900
