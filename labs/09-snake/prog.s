// Snake - Capstone like easy6502 snake (bare-metal RISC-V)
// Uses framebuffer 0x200 (32x32), random at 0xFE, key at 0xFF (WASD)
// Simplified: snake moves, apple random, no self-collision yet

.equ FB, 0x200
.equ RAND, 0xFE
.equ KEY, 0xFF

.section .text
.globl _start
_start:
    li s0, FB
    // init snake at (16,16) head, (15,16) body - stored at 0x300
    li t1, 0x300
    li t0, 16
    sw t0, 0(t1)       // X0=16
    sw t0, 4(t1)       // Y0=16
    li t0, 15
    sw t0, 8(t1)       // X1=15
    li t0, 16
    sw t0, 12(t1)      // Y1=16
    li t0, 1
    sw t0, 16(t1)      // DIR=1 (right) at 0x310
    jal ra, gen_apple

loop:
    jal ra, read_keys
    jal ra, update_snake
    jal ra, draw
    li t0, 200
spin:
    addi t0, t0, -1
    bne t0, zero, spin
    j loop

read_keys:
    li t0, KEY
    lbu t1, 0(t0)
    li t2, 0x77       // 'w'
    beq t1, t2, set_up
    li t2, 0x64       // 'd'
    beq t1, t2, set_right
    li t2, 0x73       // 's'
    beq t1, t2, set_down
    li t2, 0x61       // 'a'
    beq t1, t2, set_left
    ret
set_up:    li t1, 0; li t0, 0x310; sw t1, 0(t0); ret
set_right: li t1, 1; li t0, 0x310; sw t1, 0(t0); ret
set_down:  li t1, 2; li t0, 0x310; sw t1, 0(t0); ret
set_left:  li t1, 3; li t0, 0x310; sw t1, 0(t0); ret

gen_apple:
    li t0, RAND
    lbu t1, 0(t0)
    andi t1, t1, 0x1F  // 0-31
    li t0, 0x308
    sw t1, 0(t0)
    li t0, RAND
    lbu t1, 0(t0)
    andi t1, t1, 0x1F
    li t0, 0x30C
    sw t1, 0(t0)
    ret

update_snake:
    li t1, 0x300
    lw t0, 0(t1)       // X head
    lw t2, 4(t1)       // Y head
    li t3, 0x310
    lw t4, 0(t3)       // DIR
    beq t4, zero, up
    li t5, 1
    beq t4, t5, right
    li t5, 2
    beq t4, t5, down
    // left
    addi t0, t0, -1
    j store_head
up:    addi t2, t2, -1; j store_head
right: addi t0, t0, 1; j store_head
down:  addi t2, t2, 1
store_head:
    andi t0, t0, 0x1F
    andi t2, t2, 0x1F
    sw t0, 0(t1)
    sw t2, 4(t1)
    ret

draw:
    li t0, FB
    li t1, 0x308
    lw t2, 0(t1)       // apple X
    li t1, 0x30C
    lw t3, 0(t1)       // apple Y
    slli t3, t3, 5     // y*32
    add t3, t3, t2
    add t3, t3, t0
    li t4, 2
    sb t4, 0(t3)       // apple red
    li t1, 0x300
    lw t2, 0(t1)       // head X
    lw t3, 4(t1)       // head Y
    slli t3, t3, 5
    add t3, t3, t2
    add t3, t3, t0
    li t4, 1
    sb t4, 0(t3)       // head white
    ret
