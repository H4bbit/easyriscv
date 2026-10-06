# First pixel: li is magic, sb draws, j . halts
# Draws 3 pixels on the 32x32 framebuffer at 0x200 (1 byte/pixel).
# li t0, N just works here (HOW it works is chapter 10, not today).
.section .text
.globl _start
_start:
    li t0, 1          # white (MAGIC for now: how it works is chapter 10)
    li t1, 0x200      # framebuffer base
    sb t0, 0(t1)      # pixel (0,0) - 1 byte per pixel
    li t0, 5          # green
    sb t0, 1(t1)      # pixel (1,0)
    li t0, 8          # orange
    sb t0, 2(t1)      # pixel (2,0)
    j .               # halt: jump to self, pc stops
