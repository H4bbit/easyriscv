// First program - easy6502 LDA/STA equivalent in RISC-V
// Draws 3 pixels on the 32x32 framebuffer at 0x200 (same map as 6502 $0200)
.section .text
.globl _start
_start:
    li t0, 1          // white (palette 1)
    li t1, 0x200
    sb t0, 0(t1)      // pixel (0,0) - 1 byte per pixel like easy6502 $0200
    li t0, 5          // green
    sb t0, 1(t1)      // pixel (1,0)
    li t0, 8          // orange
    sb t0, 2(t1)      // pixel (2,0)
    j .               // halt (jal 0)
