// Solution: WITHOUT saving ra, the nested call clobbers the return address
// Compare with labs/07-jumping/prog.s: outer's ra is overwritten by
// 'jal ra, inner', so outer returns to the wrong place.
.section .text
.globl _start
_start:
    li t0, 0
    jal ra, inc_one
    li t1, 0x200
    sb t0, 0(t1)       // FB[0] = 1
    j after
    li t0, 99          // skipped
after:
    addi t0, t0, 4    // t0 = 5
    sb t0, 1(t1)      // FB[1] = 5
    li sp, 0x900
    jal ra, outer
    sb t0, 2(t1)      // never reached correctly: ra was clobbered
    j .

inc_one:
    addi t0, t0, 1
    jalr zero, 0(ra)

outer:
    // NO sw ra / lw ra here (broken version)
    addi t0, t0, 1    // t0 5->6
    jal ra, inner     // clobbers ra: return to _start is lost
    jalr zero, 0(ra)  // returns into the middle of outer, not to _start

inner:
    addi t0, t0, 9    // t0 6->15
    jalr zero, 0(ra)
// Expected: FB = 1 5 0 — FB[2] is never written. After 'jal ra, inner',
// ra points past the jal (0x44) instead of back to _start (0x2C),
// so outer's ret lands at 0x44 and the VM runs off into zeroed memory.
