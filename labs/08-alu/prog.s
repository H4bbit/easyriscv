# ALU - AND/OR/XOR bitwise ops and immediate masking
# First half is canonical (and/andi, or, xori with explicit -1);
# second half is the shorthand spellings (not, neg, seqz, snez)
# for the same hardware. FB = dice 2-5, 15, 0, then fixed 5, 9, 0, 1.

.section .text
.globl _start
_start:
    # load random byte from 0xFE
    li t0, 0xFE
    lbu t1, 0(t0)      # t1 = random 0-255
    andi t1, t1, 0x03  # mask low 2 bits -> 0-3
    addi t1, t1, 2     # 2-5
    # write result to framebuffer
    li t2, 0x200
    sb t1, 0(t2)       # FB[0] = 2-5

    # OR example: combine nibbles
    li t3, 0x0A        # 00001010
    li t4, 0x05        # 00000101
    or t5, t3, t4      # 00001111 = 15
    sb t5, 1(t2)       # FB[1] = 15

    # XOR toggle, canonical spelling
    li t6, 0xFF
    xori t6, t6, 0x0F  # 0xF0 -> low nibble 0
    sb t6, 2(t2)       # FB[2] = 0

    # --- shorthand half: same ALU, alias spellings ---
    li t3, 0x0A        # 00001010
    not t4, t3        # = xori t4, t3, -1 -> 0xFFFFFFF5
    sb t4, 3(t2)       # FB[3] = 5

    li t5, 7
    neg t6, t5        # = sub t6, x0, t5 -> -7 = 0xFFFFFFF9
    sb t6, 4(t2)       # FB[4] = 9

    seqz s0, t5       # = sltiu s0, t5, 1 -> 0 (7 != 0)
    sb s0, 5(t2)       # FB[5] = 0
    snez s1, t5       # = sltu s1, x0, t5 -> 1 (0 < 7 unsigned)
    sb s1, 6(t2)       # FB[6] = 1

    j .

# Exercises (see book/11-alu.md):
# 1. Change mask 0x03 to 0x07 -> range 0-7, then +2 -> 2-9
# 2. Use slli/srli to extract high nibble (>>4)
# 3. sub + slt by hand (10 vs 3, both directions)
# 4. Translate not/neg/seqz/snez to canonical and back; disasm must not change
