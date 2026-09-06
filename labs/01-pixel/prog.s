// First program - easy6502 LDA/STA equivalent in RISC-V
// Draws 3 pixels on the 32x32 framebuffer at 0x200 (same map as 6502 $0200)
.section .text
.globl _start
_start:
    li t0, 1          // white
    li t1, 0x200
    sw t0, 0(t1)      // pixel (0,0)
    li t0, 5
    sw t0, 4(t1)      // pixel (1,0) - one word per pixel (4 bytes) for simplicity
    li t0, 8
    sw t0, 8(t1)      // pixel (2,0)
    j .               // halt (jal 0)
