// Stack - manual push/pop via sp
// Demonstrates push/pop via sp (software convention, grows down)
// Stack grows down from 0x1FC (top of safe RAM, MEM_SIZE=0x1000)

.section .text
.globl _start
_start:
    li sp, 0x1FC       // init stack pointer (like SP=$FF -> $01FF)

    // push 3 values (PHA equivalent)
    li t0, 11
    addi sp, sp, -4
    sw t0, 0(sp)       // push 11

    li t0, 22
    addi sp, sp, -4
    sw t0, 0(sp)       // push 22

    li t0, 33
    addi sp, sp, -4
    sw t0, 0(sp)       // push 33 (top)

    // pop in reverse and write to framebuffer at 0x200 (1 byte/pixel)
    li t1, 0x200
    lw t0, 0(sp)       // pop 33
    addi sp, sp, 4
    sb t0, 0(t1)

    lw t0, 0(sp)       // pop 22
    addi sp, sp, 4
    sb t0, 1(t1)

    lw t0, 0(sp)       // pop 11
    addi sp, sp, 4
    sb t0, 2(t1)

    // Stack should be back at 0x1FC
    // Write sp low nibble to FB[3] for visual check
    andi t0, sp, 0xF
    sb t0, 3(t1)

    j .

// Exercises:
// 1. What happens if you forget addi sp,sp,4 after lw? (stack leak)
// 2. Try pushing 8 values and popping - draw a mirrored pattern (push loop then pop loop)
// 3. Use sp to save ra before a jal (preview of next lab)
