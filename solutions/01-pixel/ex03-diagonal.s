// Solution: diagonal from (0,0) to (7,7)
.section .text
.globl _start
_start:
    li t1, 0x200
    li t0, 1
    sb t0, 0(t1)      // (0,0)
    sb t0, 33(t1)     // (1,1) 32+1
    sb t0, 66(t1)     // (2,2)
    sb t0, 99(t1)
    sb t0, 132(t1)
    sb t0, 165(t1)
    sb t0, 198(t1)
    sb t0, 231(t1)    // (7,7)
    j .
