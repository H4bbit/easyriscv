# Solution: blt instead of bne - exits after one iteration
# t0 goes 8->7; 7<3 (signed) is false, so the loop exits at once.
# Translation twin: bgt with operands swapped.
#   blt t0, t2, loop   means  t0 < 3   (canonical)
#   bgt t2, t0, loop   means  3 > t0   (shorthand, same encoding)
# disasm prints blt for both (0x614 and 0x618). Contrast with bne, which loops until t0==3.
.section .text
.globl _start
_start:
    li t0, 8
    li t1, 0x200
    li t2, 3
loop:
    addi t0, t0, -1
    sb t0, 0(t1)
    blt t0, t2, loop   # taken only while t0 < 3: never here
    bgt t2, t0, loop   # shorthand twin: bgt rs, rt = blt rt, rs (never taken either)
    sb t0, 1(t1)
    j .
# Expected: halt with t0=7, FB[0]=FB[1]=7 (vs 3 3 with bne)
