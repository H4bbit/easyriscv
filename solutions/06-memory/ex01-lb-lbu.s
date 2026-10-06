# Solution: lb sign-extends, lbu zero-extends (byte 0xAB at 0x00C)
.section .text
.globl _start
_start:
    li t0, 0x000
    li t1, 0xAB
    sb t1, 12(t0)     # RAM[0x00C] = 0xAB
    lb t4, 12(t0)     # sign-extend: 0xFFFFFFAB
    lbu t5, 12(t0)    # zero-extend: 0x000000AB
    sw t4, 0(t0)      # RAM[0x000] = 4294967211
    sw t5, 4(t0)      # RAM[0x004] = 171
    j .
# Expected: RAM 0x000 = 4294967211 171 ... (same low nibble 0xB, different words)
