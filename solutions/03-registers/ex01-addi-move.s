# Solution: move t0 to a0 both ways (canonical addi, shorthand mv)
# Expected: halt with a0=42. disasm prints `mv` for BOTH spellings:
#   addi a0, t0, 0  ->  mv a0, t0
.section .text
.globl _start
_start:
    li t0, 42
    addi a0, t0, 0    # canonical: a0 = t0 + 0
    mv a0, t0         # shorthand: same encoding, same a0=42
    j .
# Expected: halt with a0=42
