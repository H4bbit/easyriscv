// Solution: print digit '5' via ecall (a7=1, a0=char)
.section .text
.globl _start
_start:
    li a0, 53          // '5'
    li a7, 1
    ecall
    j .
// Expected: stdout shows '5'
