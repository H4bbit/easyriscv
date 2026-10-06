# ALU - AND/OR/XOR bitwise ops and immediate masking
# Demonstrates andi/ori/xori and bit masking (e.g. AND #0x03 style masks)

.section .text
.globl _start
_start:
    # load random byte from 0xFE (sysRandom)
    li t0, 0xFE
    lbu t1, 0(t0)      # t1 = random 0-255
    andi t1, t1, 0x03  # mask low 2 bits -> 0-3
    addi t1, t1, 2     # 2-5 (like apple Y 2-5)
    # write result to framebuffer
    li t2, 0x200
    sb t1, 0(t2)       # FB[0] = 2-5

    # OR example: combine nibbles
    li t3, 0x0A        # 00001010
    li t4, 0x05        # 00000101
    or t5, t3, t4      # 00001111 = 15
    sb t5, 1(t2)       # FB[1] = 15

    # XOR toggle
    li t6, 0xFF
    xori t6, t6, 0x0F  # 0xF0 -> low nibble 0
    sb t6, 2(t2)       # FB[2] = 0

    j .

# Exercises:
# 1. Change mask 0x03 to 0x07 -> range 0-7
# 2. Use slli/srli to extract high nibble (>>4)
# 3. Combine random X in 0xFE and Y masked to make pixel coord
