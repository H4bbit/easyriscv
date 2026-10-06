# Registers

You've seen `t0` and `pc` change. What do they mean? The ncurses UI `w_regs` shows:

```
zero:00000000 ra:00000000 sp:00000000 gp:00000000
 t0:00000000  t1:00000000  t2:00000000  t3:00000000
 s0:00000000  s1:00000000  a0:00000000  a1:00000000
 a2:00000000  a3:00000000  a4:00000000  a5:00000000
 a6:00000000  a7:00000000  pc:00000000  RUN
```

## The Register File

RISC-V has 32 32-bit registers `x0-x31` with ABI names:

* `zero (x0)` — always 0. Writes are ignored. Use it to compare against zero (`beq rs, zero, label`) or as source for `li`: `li t0, 5` assembles to `addi t0, zero, 5`.
* `ra (x1)` — return address for `jal`. Holds where to return after a function call.
* `sp (x2)` — stack pointer. You set it yourself (`li sp, 0x1FC`). It grows down with `addi sp, sp, -4`.
* `gp (x3), tp (x4)` — global and thread pointers, rarely used bare-metal.
* `t0-t6 (x5-x7, x28-x31)` — temporaries, caller-saved. Use them for scratch values.
* `s0-s11 (x8-x9, x18-x27)` — saved registers, callee-saved. `s0` is also `fp`.
* `a0-a7 (x10-x17)` — arguments and return value. `a0` holds the Fibonacci result in `02-fib`.

All registers hold 32 bits. `pc` is the program counter — the address of the current instruction. Our binary loads at `0x600`, so `pc` starts at `0x600` and advances by 4.

There is no flags register. Branches do the compare directly (see next chapter).

A full list of the RV32I instruction set — arguments, registers, encodings — is in the [RISC-V ISA Manual, Volume I: Unprivileged](https://docs.riscv.org/reference/isa/unpriv/unpriv-index.html) (RV32I chapter), the [RISC-V Assembly Programmer's Manual](https://github.com/riscv-non-isa/riscv-asm-manual) (syntax and pseudo-ops), and the [RISC-V Green Card](https://dejazzer.com/coen2710/lectures/RISC-V-Reference-Data-Green-Card.pdf) (one-page opcode/ABI reference). Keep them open beside the debugger — they are your bible.

## Try It

```bash
just debug 02-fib
# step, watch t0/t1/t2/a0 and pc (blt is a preview: branches are covered in [05-branching](05-branching.md))
```

`02-fib` computes Fibonacci(7) = 13 with a counted loop — canonical copies first, shorthand after:

```asm
    li a0, 7          # N = 7
    li t0, 0          # F(0)
    li t1, 1          # F(1)
loop:
    add t3, t0, t1    # 3-operand: t3 = t0 + t1
    addi t0, t1, 0    # canonical copy: t0 = t1 + 0
    addi t1, t3, 0    # canonical copy: t1 = t3 + 0
    blt t2, a0, loop
```

`addi rd, rs, 0` copies `rs` to `rd` — that is the whole trick behind the shorthand `mv rd, rs` (`just disasm 02-fib` shows both as `addi`). From here on the book writes `mv` and you read it as `addi ..., 0`.

## Exercises

1. Rewrite the loop copies with `mv` (`mv t0, t1` for `addi t0, t1, 0`) and confirm `a0=13` is unchanged. ([solution](../solutions/04-registers/ex01-addi-move.s))
2. What happens if you write to `zero`? Try `addi zero, t0, 0` then `addi a0, zero, 0`. ([solution](../solutions/04-registers/ex02-write-zero.s))
3. Compare `sp` after `07-stack` (preview: the stack pointer is covered in [07-stack](07-stack.md)): run it headless and check the final `t0` (which holds `sp & 0xF`).

Next: [05-branching.md](05-branching.md)
