// Solution: print "HI" via 0x1000 stores
.section .text
.globl _start
_start:
    li t0, 0x1000
    li t1, 72          // 'H'
    sw t1, 0(t0)
    li t1, 73          // 'I'
    sw t1, 0(t0)
    j .
// Expected: stdout shows 'HI'
