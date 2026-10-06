# First program - load immediates and store bytes to the framebuffer
# Draws 3 pixels on the 32x32 framebuffer at 0x200 (1 byte/pixel)
# Step 1 is canonical (addi, jal x0), step 2 is the shorthand (li, j, nop).
.section .text
.globl _start
_start:
    addi t0, zero, 1      # canonical: t0 = 1 (see li below)
    addi t1, zero, 0x200 # canonical: t1 = 0x200
    sb t0, 0(t1)      # pixel (0,0) - 1 byte per pixel
    li t0, 5          # shorthand for addi t0, zero, 5: green
    sb t0, 1(t1)      # pixel (1,0)
    li t0, 8          # shorthand for addi t0, zero, 8: orange
    sb t0, 2(t1)      # pixel (2,0)
    nop               # shorthand for addi x0, x0, 0: one quiet cycle
    jal x0, .         # halt: jump to self (j . is shorthand)
