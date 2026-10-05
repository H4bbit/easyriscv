// Solution: mirrored pattern via push loop then pop loop
// Push 0..7, pop in reverse and draw to framebuffer (mirror effect)
.section .text
.globl _start
_start:
    li sp, 0x900
    li t1, 0x200
    li t2, 0          // counter = 0
push:
    addi sp, sp, -4
    sw t2, 0(sp)      // push counter
    addi t2, t2, 1
    li t3, 8
    blt t2, t3, push  // push 0..7
    li t2, 0          // pixel index = 0
pop:
    lw t0, 0(sp)
    addi sp, sp, 4
    add t3, t1, t2
    sb t0, 0(t3)      // draw popped value (7..0)
    addi t2, t2, 1
    li t3, 8
    blt t2, t3, pop
    j .
// Expected: FB[0..7] = 7 6 5 4 3 2 1 0, sp back at 0x900
