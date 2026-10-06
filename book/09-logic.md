# ALU

> Renamed: old `11-alu` → new `09-logic`.


RISC-V arithmetic is three-operand with no flags: the result goes to a
register, and the next instruction decides what to do with it. This lab
tours the whole integer ALU — logic, shifts, compare — using the
framebuffer as a truth table. First half is canonical, second half is
the shorthand spelling of the same hardware.

## Setup: `09-logic` needs `0xFE` first

Open `labs/09-logic/prog.s`: the first instruction reads the random port:

```asm
    li t0, 0xFE
    lbu t1, 0(t0)      # random 0-255
```

If you haven't read [08-dice.md](08-dice.md), do that first — `0xFE`
returns a new random byte on every instruction. From here on we assume
you can dice, mask, and print.

Run:

```bash
just debug 09-logic
```

Source (`labs/09-logic/prog.s`):

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

## Shorthand half: NOT / NEG / set-if

The second half of `prog.s` re-spells three canonical shapes with aliases:

* `not t4, t3` is `xori t4, t3, -1` — flip every bit (`0x0A` → `0xFFFFFFF5`, `FB[3] = 5`).
* `neg t6, t5` is `sub t6, x0, t5` — two's complement (`7` → `-7` = `0xFFFFFFF9`, `FB[4] = 9`).
* `seqz s0, t5` is `sltiu s0, t5, 1` — `1` only if `t5 == 0` (`FB[5] = 0`); `snez s1, t5` is `sltu s1, x0, t5` — `1` unless `t5 == 0` (`FB[6] = 1`).
* Same family, used across the labs: `sltz rs` is `slt rd, rs, x0` (negative?) and `sgtz rs` is `slt rd, x0, rs` (positive?).

`just disasm 09-logic` is the answer key: canonical `xori` at `0x62c` stays `xori`, while the alias spellings print as `not` (`0x638`), `neg` (`0x644`), `seqz` (`0x64c`), `snez` (`0x654`).

## Shifts and compare (used across the labs)

Two families you have already seen but never isolated:

* `slli t4, t2, 2` — shift left by 2 = ×4. This is how `07-words-vectors` scales
  an index to words, and how the Snake capstone computes `y*32` (`slli t3, t3, 5`).
  Right shifts: `srli` (zeros in) vs `srai` (sign bit in).
* `slt t5, t3, t4` — `1` if `t3 < t4` (signed), else `0`. Branches like `blt`
  do this compare internally; `slt` keeps the `0/1` in a register instead.
  `sltu`/`slti` are the unsigned/immediate forms. `sub` is the matching
  counterpart of `add`.

## Try It

Step and watch `t1`: it starts random, then snaps to `2-5` after the mask.
`FB[0]` is non-deterministic (dice), `FB[1]=15` and `FB[2]=0` are fixed — and the shorthand half is fully fixed: `FB[3]=5`, `FB[4]=9`, `FB[5]=0`, `FB[6]=1`.
Headless prints e.g. `Framebuffer 0x200: 3 15 0 5 9 0 1 0` (first value varies per run).

## Translation exercise

Translate both ways and confirm `disasm` does not change: `xori t4, t3, -1` ↔ `not t4, t3`, `sub t6, x0, t5` ↔ `neg t6, t5`, `sltiu s0, t5, 1` ↔ `seqz s0, t5`, `sltu s1, x0, t5` ↔ `snez s1, t5`.

## Exercises

1. Change the mask `0x03` to `0x07`: what range lands in `FB[0]`? ([solution](../solutions/09-logic/ex01-mask7.s))
2. Extract the high nibble of a random byte: `srli` right by 4. ([solution](../solutions/09-logic/ex02-high-nibble.s))
3. `sub` + `slt` by hand: `t3=10, t4=3` — compute `t3-t4` and `slt` both directions. ([solution](../solutions/09-logic/ex03-sub-slt.s))
4. Write `sltz`/`sgtz` over a negative and a positive value, then read back the canonical `slt` forms in `disasm`.

Next: [07-words-vectors.md](07-words-vectors.md)
