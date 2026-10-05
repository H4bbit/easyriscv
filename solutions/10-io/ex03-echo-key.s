// Solution: echo last key's low nibble to FB[1]
// In debug mode, press a key then step: the pixel takes the key's color.
// Headless, 0xFF stays 0 so FB[1] = 0.
.section .text
.globl _start
_start:
    li t0, 0xFF
    lbu t1, 0(t0)      // last key ASCII (0 if none)
    andi t1, t1, 0x0F  // low nibble -> color
    li t2, 0x200
    sb t1, 1(t2)       // FB[1] = key color
    j .
// Expected headless: FB[1]=0; in debug after pressing 'w' (0x77): FB[1]=7
