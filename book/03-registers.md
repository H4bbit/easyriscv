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
* `sp (x2)` — stack pointer. You set it yourself (`li sp, 0x900`). It grows down with `addi sp, sp, -4`.
* `gp (x3), tp (x4)` — global and thread pointers, rarely used bare-metal.
* `t0-t6 (x5-x7, x28-x31)` — temporaries, caller-saved. Use them for scratch values.
* `s0-s11 (x8-x9, x18-x27)` — saved registers, callee-saved. `s0` is also `fp`.
* `a0-a7 (x10-x17)` — arguments and return value. `a0` holds the Fibonacci result in `02-fib`.

All registers hold 32 bits. `pc` is the program counter — the address of the current instruction. Our binary loads at `0x0`, so `pc` starts at `0x0` and advances by 4.

There is no flags register. Branches do the compare directly (see next chapter).

## Try It

```bash
just debug 02-fib
# step, watch t0/t1/t2/a0 and pc
```

In `02-fib`, `li t0, 0` sets `t0=0`, `add t3, t0, t1` reads two registers and writes a third — RISC-V is 3-operand.

## Exercises

1. Move `t0` to `a0` without `mv`. Hint: `addi a0, t0, 0`.
2. What happens if you write to `zero`? Try `li zero, 5` then `mv a0, zero`.
3. Compare `sp` after `06-stack`: run it headless and check the final `t0` (which holds `sp & 0xF`).

Next: [04-branching.md](04-branching.md)
