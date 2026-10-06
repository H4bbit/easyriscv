# Snake - capstone game (bare-metal RISC-V)
# Framebuffer 0x200 (32x32, 1 byte/pixel), random at 0xFE, key at 0xFF (WASD)
# Game state in zero-page RAM at 0x00: code lives at 0x600 above the screen,
# state below it, so the full 32x32 arena is playable.
# Layout: 0x00 DIR, 0x04 LEN (segment count), 0x08 APPLE (pixel addr),
#         0x0C seg[0]=head, 0x10 seg[1], ... (one word per segment).
# Directions use one bit each (like the reference): 1=up 2=right 4=down 8=left.
# Opposite pairs share one AND test: up|down, right|left.

.equ FB,    0x200
.equ RAND,   0xFE
.equ KEY,    0xFF
.equ STATE,  0x00
.equ DIR,    0x0
.equ LEN,    0x4
.equ APPLE,  0x8
.equ SEGS,   0xC

.equ UP,     1
.equ RIGHT,  2
.equ DOWN,   4
.equ LEFT,   8

.section .text
.globl _start
_start:
    li sp, 0x1FC      # stack top (grows down, code is far above at 0x600)
    jal ra, init
    jal ra, loop
    j .               # loop never returns; halt if it ever does

# init + loop subroutines, called once each from _start
init:
    addi sp, sp, -4   # non-leaf: save ra before nested calls (see 07-jumping)
    sw ra, 0(sp)
    jal ra, init_snake
    jal ra, gen_apple
    lw ra, 0(sp)
    addi sp, sp, 4
    ret

init_snake:
    li t0, STATE
    li t1, RIGHT
    sw t1, DIR(t0)    # start moving right
    li t1, 2
    sw t1, LEN(t0)    # 2 segments: head + 1 body
    li t1, 0x411
    sw t1, SEGS(t0)   # head = (17,16), middle of screen
    li t1, 0x410
    li t2, SEGS+4
    add t2, t2, t0
    sw t1, 0(t2)      # body = (16,16)
    ret

gen_apple:
    li t0, RAND
    lbu t1, 0(t0)     # random byte -> X 0-31
    andi t1, t1, 0x1F
    lbu t2, 0(t0)     # another random byte (0xFE changes every step)
    andi t2, t2, 0x1F # Y 0-31 (whole screen: code is at 0x600, out of the way)
    slli t2, t2, 5    # Y*32
    add t1, t1, t2    # Y*32+X
    li t2, FB
    add t1, t1, t2    # pixel address 0x200-0x5FF
    li t0, STATE
    sw t1, APPLE(t0)
    ret

# main game loop: input, update state, render
loop:
    jal ra, read_keys
    jal ra, check_collision
    jal ra, update_snake
    jal ra, draw_apple
    jal ra, draw_snake
    li t0, 400
spin:
    addi t0, t0, -1
    bne t0, zero, spin
    j loop

read_keys:
    li t0, KEY
    lbu t1, 0(t0)     # ASCII of last key (0 if none)
    li t2, 0x77       # 'w'
    beq t1, t2, key_up
    li t2, 0x64       # 'd'
    beq t1, t2, key_right
    li t2, 0x73       # 's'
    beq t1, t2, key_down
    li t2, 0x61       # 'a'
    beq t1, t2, key_left
    ret
key_up:
    li t0, STATE
    lw t1, DIR(t0)
    andi t2, t1, DOWN # moving down? (opposite of up: reject reversal)
    bne t2, zero, illegal
    li t1, UP
    sw t1, DIR(t0)
    ret
key_right:
    li t0, STATE
    lw t1, DIR(t0)
    andi t2, t1, LEFT
    bne t2, zero, illegal
    li t1, RIGHT
    sw t1, DIR(t0)
    ret
key_down:
    li t0, STATE
    lw t1, DIR(t0)
    andi t2, t1, UP
    bne t2, zero, illegal
    li t1, DOWN
    sw t1, DIR(t0)
    ret
key_left:
    li t0, STATE
    lw t1, DIR(t0)
    andi t2, t1, RIGHT
    bne t2, zero, illegal
    li t1, LEFT
    sw t1, DIR(t0)
    ret
illegal:
    ret

check_collision:
    addi sp, sp, -4   # non-leaf: check_apple calls gen_apple
    sw ra, 0(sp)
    jal ra, check_apple
    jal ra, check_snake
    lw ra, 0(sp)
    addi sp, sp, 4
    ret

check_apple:
    addi sp, sp, -4
    sw ra, 0(sp)
    li t0, STATE
    lw t1, APPLE(t0)
    lw t2, SEGS(t0)   # head address
    bne t1, t2, no_apple
    lw t1, LEN(t0)    # eat apple: grow one segment...
    addi t1, t1, 1
    sw t1, LEN(t0)
    jal ra, gen_apple # ...and place a new apple
no_apple:
    lw ra, 0(sp)
    addi sp, sp, 4
    ret

check_snake:
    li t0, STATE
    lw t1, SEGS(t0)   # head address
    lw t2, LEN(t0)    # segment count
    li t3, 1          # i = 1 (skip head)
cs_loop:
    bge t3, t2, no_snake
    slli t4, t3, 2
    add t4, t4, t0    # STATE + i*4
    li t5, SEGS
    add t4, t4, t5
    lw t4, 0(t4)      # seg[i]
    bne t4, t1, cs_next
    j game_over       # head hit body
cs_next:
    addi t3, t3, 1
    j cs_loop
no_snake:
    ret

update_snake:
    li t0, STATE
    lw t1, LEN(t0)
    addi t1, t1, -1   # i = LEN-1 (start at tail)
us_loop:
    blez t1, move_head # shift seg[1..LEN-1] only; seg[0] is rewritten below
    slli t2, t1, 2
    add t2, t2, t0    # STATE + i*4
    li t3, SEGS
    add t2, t2, t3    # &seg[i]
    lw t3, -4(t2)     # seg[i-1]
    sw t3, 0(t2)      # seg[i] = seg[i-1]: body follows head
    addi t1, t1, -1
    j us_loop
move_head:
    lw t1, SEGS(t0)   # head address
    lw t2, DIR(t0)
    li t3, UP
    beq t2, t3, go_up
    li t3, RIGHT
    beq t2, t3, go_right
    li t3, DOWN
    beq t2, t3, go_down
    addi t1, t1, -1   # left: column - 1
    andi t3, t1, 0x1F
    li t4, 0x1F
    beq t3, t4, game_over # wrapped past column 0
    j store_head
go_up:
    addi t1, t1, -32  # row - 1
    li t3, FB
    blt t1, t3, game_over # above row 0
    j store_head
go_right:
    addi t1, t1, 1    # column + 1
    andi t3, t1, 0x1F
    beq t3, zero, game_over # wrapped past column 31
    j store_head
go_down:
    addi t1, t1, 32   # row + 1
    li t3, 0x600
    bge t1, t3, game_over # below row 31 (0x200+1024 = 0x600)
store_head:
    sw t1, SEGS(t0)
    ret

draw_apple:
    li t0, RAND
    lbu t1, 0(t0)     # random color (low nibble shows)
    li t0, STATE
    lw t2, APPLE(t0)
    sb t1, 0(t2)
    ret

draw_snake:
    li t0, STATE
    lw t1, LEN(t0)
    addi t1, t1, -1   # tail index
    slli t1, t1, 2
    add t1, t1, t0    # STATE + tail*4
    li t2, SEGS
    add t1, t1, t2
    lw t2, 0(t1)      # tail pixel address
    sb zero, 0(t2)    # erase tail (black)
    lw t2, SEGS(t0)   # head pixel address
    li t3, 1
    sb t3, 0(t2)      # paint head white
    ret

game_over:
    li t0, STATE
    lw t1, SEGS(t0)
    li t2, 2
    sb t2, 0(t1)      # red head marks the crash
halt:
    jal x0, halt      # halt: jump to self (pc stops, like j . in 01-pixel)
