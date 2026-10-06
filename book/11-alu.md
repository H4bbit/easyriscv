# ALU

RISC-V arithmetic is three-operand with no flags: the result goes to a
register, and the next instruction decides what to do with it. This lab
tours the whole integer ALU — logic, shifts, compare — using the
framebuffer as a truth table.

## Setup: `08-alu` needs `0xFE` first

Open `labs/08-alu/prog.s`: the first instruction reads the random port:

```asm
    li t0, 0xFE
    lbu t1, 0(t0)      # random 0-255
```

If you haven't read [10-io.md](10-io.md), do that first — `0xFE`
returns a new random byte on every instruction. From here on we assume
you can dice, mask, and print.

Run:

```bash
just debug 08-alu
```

Source (`labs/08-alu/prog.s`):

```asm
    lbu t1, 0(t0)      # random 0-255
    andi t1, t1, 0x03  # mask low 2 bits -> 0-3
    addi t1, t1, 2     # 2-5
    sb t1, 0(t2)       # FB[0] = 2-5
    or t5, t3, t4      # 0x0A | 0x05 = 15
    sb t5, 1(t2)       # FB[1] = 15
    xori t6, t6, 0x0F
    sb t6, 2(t2)       # FB[2] = 0
```

## Logic: AND / OR / XOR

Each bit is computed independently (`1` only where the rule says):

* `and t5, t3, t4` — `1` where both are `1`. Used as a **mask**: `andi t1, t1, 0x03`
  keeps only the low 2 bits (`00000011`), turning `0-255` into `0-3`.
* `or` — `1` where either is `1`. Used to **combine**: `0x0A | 0x05 = 0x0F` (15).
* `xor` — `1` where they differ. Used to **toggle**: `xori t6, t6, 0x0F` flips
  the low nibble (`0xFF` becomes `0xF0`, low nibble `0`).

Register-register (`and`/`or`/`xor`) and immediate (`andi`/`ori`/`xori`)
forms both exist. The `i` form sign-extends a 12-bit immediate — fine for
masks like `0x03` or `0x0F`.

## Shifts and compare (used across the labs)

Two families you have already seen but never isolated:

* `slli t4, t2, 2` — shift left by 2 = ×4. This is how `03-fib-ram` scales
  an index to words, and how the Snake capstone computes `y*32` (`slli t3, t3, 5`).
  Right shifts: `srli` (zeros in) vs `srai` (sign bit in).
* `slt t5, t3, t4` — `1` if `t3 < t4` (signed), else `0`. Branches like `blt`
  do this compare internally; `slt` keeps the `0/1` in a register instead.
  `sltu`/`slti` are the unsigned/immediate forms. `sub` is the matching
  counterpart of `add`.

## Try It

Step and watch `t1`: it starts random, then snaps to `2-5` after the mask.
`FB[0]` is non-deterministic (dice), `FB[1]=15` and `FB[2]=0` are fixed —
that split is the point: mask dice, assert logic.

## Exercises

1. Change the mask `0x03` to `0x07`: what range lands in `FB[0]`? ([solution](../solutions/11-alu/ex01-mask7.s))
2. Extract the high nibble of a random byte: `srli` right by 4. ([solution](../solutions/11-alu/ex02-high-nibble.s))
3. `sub` + `slt` by hand: `t3=10, t4=3` — compute `t3-t4` and `slt` both directions. ([solution](../solutions/11-alu/ex03-sub-slt.s))

Next: [12-fib-ram.md](12-fib-ram.md)
