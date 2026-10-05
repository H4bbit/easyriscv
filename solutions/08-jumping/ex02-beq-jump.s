// Solution: beq zero, zero acts as an unconditional jump
// (comparing zero with itself is always equal, so it always branches)
.section .text
.globl _start
_start:
    li t0, 0
    jal ra, inc_one
    li t1, 0x200
    sb t0, 0(t1)       // FB[0] = 1
    beq zero, zero, after  // always taken: same as j after
    li t0, 99          // skipped
after:
    addi t0, t0, 4     // t0 = 5
    sb t0, 1(t1)       // FB[1] = 5
    j .

inc_one:
    addi t0, t0, 1
    jalr zero, 0(ra)
// Expected: FB[0]=1 FB[1]=5 (identical to j version)
