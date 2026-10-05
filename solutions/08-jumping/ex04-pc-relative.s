// Solution: la/call are PC-relative (auipc-based), j/li are not.
//
// la t1, msg loads the ADDRESS of msg via auipc+addi (works wherever
// the program is loaded); call msg_fn jumps PC-relative via jal.
// disasm shows the expansion: no la/call remains, only auipc/addi/jal.
.section .text
.globl _start
_start:
    la t0, msg        // auipc+addi: t0 = address of msg
    lw t1, 0(t0)      // t1 = *msg = 42
    li t2, 0x200
    sb t1, 0(t2)      // FB[0] = 42 & 0xF = 10
    call show         // jal: PC-relative call
    j .
show:
    sb t1, 1(t2)      // FB[1] = 10
    ret
msg:
    .word 42
// Expected: FB[0]=10 FB[1]=10 (disasm: auipc t0 + addi, jal show, ret)
