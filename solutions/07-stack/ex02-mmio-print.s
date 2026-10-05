// Solution: sw to 0x1000 prints the low byte as a char (MMIO, not RAM)
// Run headless and watch stdout: it prints 'A', nothing is stored in RAM.
.section .text
.globl _start
_start:
    li t0, 0x1000     // outside 4KB RAM (0x000-0xFFF): print-char port
    li t1, 65         // 'A'
    sw t1, 0(t0)      // prints 'A' to stdout
    j .
// Expected: stdout shows 'A', no memory change
