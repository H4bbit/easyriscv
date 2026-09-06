# Registers

You've seen `t0` and `pc` change. What do they mean? In the ncurses UI `w_regs` shows:

```
zero:00000000 ra:00000000 sp:00000000 gp:00000000
 t0:00000000  t1:00000000  t2:00000000  t3:00000000
 s0:00000000  s1:00000000  a0:00000000  a1:00000000
 a2:00000000  a3:00000000  a4:00000000  a5:00000000
 a6:00000000  a7:00000000  pc:00000000  RUN
```

## The Register File

RISC-V has 32 32-bit registers `x0-x31` with ABI names:

* `zero (x0)` — always 0. Writes are ignored. Like 6502's lack of zero register — here you use `beq rs, zero, label` for compare-against-zero. `x0` as `rs1` makes `li` pseudo: `li t0, 5` is `addi t0, zero, 5`.
* `ra (x1)` — return address for `jal`. Like 6502's stack-saved PC for `JSR/RTS`, but explicit.
* `sp (x2)` — stack pointer. You set it yourself (`li sp, 0x900`). Grows down with `addi sp, sp, -4`. 6502's `SP` at `$01FF` is automatic; here you manage it (see `06-stack`).
* `gp (x3), tp (x4)` — rarely used bare-metal.
* `t0-t6 (x5-x7, x28-x31)` — temporaries, caller-saved. Like `A` but you have 7 of them.
* `s0-s11 (x8-x9, x18-x27)` — saved registers, callee-saved. `s0` is also `fp`.
* `a0-a7 (x10-x17)` — arguments and return value. `a0` holds the Fibonacci result in `02-fib`. Like 6502's `A` on return.

All registers hold 32 bits, not one byte. `pc` is the program counter — the "line number" the CPU is at. Our binary loads at `0x0`, so `pc` starts at `0x0` (vs 6502's `0x0600`).

No `P` flags byte. 6502's `NV-BDIZC` flags are implicit in RISC-V: a `blt` does `if (rs1 < rs2)` directly, no `Z` flag set by `CMP`.

## Try It

```bash
just debug 02-fib
# step, watch t0/t1/t2/a0 and pc
```

In `02-fib`, `li t0, 0` sets `t0=0`, `add t3, t0, t1` reads two registers and writes a third — 3-operand, vs 6502's `ADC` that always uses `A`.

## Exercises

1. Write a snippet that moves `t0` to `a0` without using `mv` pseudo. Hint: `addi a0, t0, 0`.
2. What happens if you write to `zero`? Try `li zero, 5` then `mv a0, zero`.
3. Compare `sp` after `06-stack`: run it headless and check final `t0` (which holds `sp & 0xF`).

Next: [04-branching.md](04-branching.md) — `beq/bne/blt` vs 6502 `BNE/BEQ/BCC`.
