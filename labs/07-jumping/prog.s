// Jumping - RISC-V jal/jalr vs 6502 JMP/JSR/RTS
// Demonstrates jal (JMP/JSR) and jalr/ret (RTS)

.section .text
.globl _start
_start:
    li t0, 0
    jal ra, inc_one   // JSR inc_one
    // t0 should be 1 here
    li t1, 0x200
    sw t0, 0(t1)       // FB[0] = 1

    j after           // JMP (jal x0)
    li t0, 99         // skipped
after:
    addi t0, t0, 4    // t0 = 5
    sw t0, 4(t1)      // FB[1] = 5

    // nested call test: save ra on stack
    li sp, 0x900
    jal ra, outer
    sw t0, 8(t1)      // FB[2] should be 15 (5 +1 +9? see below)
    j .

inc_one:
    addi t0, t0, 1
    jalr zero, 0(ra)  // RTS

outer:
    addi sp, sp, -4
    sw ra, 0(sp)
    addi t0, t0, 1    // t0 5->6
    jal ra, inner
    lw ra, 0(sp)
    addi sp, sp, 4
    jalr zero, 0(ra)

inner:
    addi t0, t0, 9    // t0 6->15
    jalr zero, 0(ra)

// Expected FB: 1, 5, 15
