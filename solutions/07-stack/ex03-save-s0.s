// Solution: save/restore s0 across a call (callee-saved demo)
.section .text
.globl _start
_start:
    li sp, 0x900
    li s0, 0x2A       // s0 = 42
    addi sp, sp, -4
    sw s0, 0(sp)      // save s0
    jal ra, clobber   // may use s0 internally
    lw s0, 0(sp)      // restore s0
    addi sp, sp, 4
    li t1, 0x200
    sb s0, 0(t1)      // FB[0] = 42 & 0xF = 10
    j .

clobber:
    li s0, 0          // clobbers s0, caller restores after return
    jalr zero, 0(ra)
// Expected: FB[0]=10 (s0 preserved as 42), sp back at 0x900
