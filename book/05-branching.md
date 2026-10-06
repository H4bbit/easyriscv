# Branching

So far we've run straight-line code. Now let's make it loop.

Run the lab:

```bash
just debug 04-branching
```

Source (`labs/04-branching/prog.s`) — canonical compare first, shorthand after:

```asm
    li t0, 8
    li t1, 0x200
    li t2, 3
loop:
    addi t0, t0, -1
    sb   t0, 0(t1)       # 1 byte/pixel at 0x200
    bne  t0, t2, loop   # canonical: branch on two registers
    sub  t3, t0, t2     # 0 once the loop really reached 3
    bnez t3, loop       # shorthand for bne t3, zero, loop: never taken
    sb   t0, 1(t1)       # final value at next pixel
    j    .               # halt (shorthand for jal x0, .)
```

## How Branches Work

`bne t0, t2, loop` means `if (t0 != t2) pc = loop`. No separate compare instruction is needed — the branch does the compare itself. Other canonical forms: `beq` (equal), `blt` (signed less), `bge`, `bltu` (unsigned), `bgeu`.

The pseudo-ops shorten the common case of comparing against zero:

* `beqz rs, label` is `beq rs, zero, label`
* `bnez rs, label` is `bne rs, zero, label`
* `blez rs, label` is `bge zero, rs, label`; `bgez` is `bge rs, zero, label`
* `bltz` is `blt rs, zero, label`; `bgtz` is `blt zero, rs, label`

Swapped-operand shorthands flip the comparison instead of adding new hardware:

* `bgt rs, rt, label` is `blt rt, rs, label` (and `ble` is `bge` with operands swapped; `bgtu`/`bleu` are the unsigned pair)

`just disasm 04-branching` is the answer key: the loop branch at `0x614` prints `bne t0, t2`, the check at `0x61c` prints `bnez t3` — the assembler folded your `bne t3, zero, loop` spelling into the alias.

The branch offset is a 13-bit PC-relative immediate assembled from the label.

## Try It

Step through `04-branching` (lab `04-branching`). `t0` goes `8→7→6→5→4→3` then falls through. `sub` confirms `t3=0`, so `bnez` never fires. The first two framebuffer bytes at `0x200` show `3 3` (headless prints `Framebuffer 0x200: 3 3 ...`), the low 4 bits of the last stores.

## Translation exercise

Translate both directions and re-check `disasm`: `bne t3, zero, loop` → `bnez t3, loop` (folds to the alias), and `bgt t0, t2, loop` → `blt t2, t0, loop` (operand swap). The encoding must not change.

## Exercises

1. Replace `bne` with `beq`. What happens? ([solution](../solutions/05-branching/ex01-beq.s))
2. Use `blt t0, t2, loop` — how does signed less-than differ? ([solution](../solutions/05-branching/ex02-blt.s))
3. Write a loop that counts up from `0` to `5` using `addi` and `bne`, then re-spell its exit test with `bnez`. ([solution](../solutions/05-branching/ex03-count-up.s))
4. Write `blez`/`bgez`/`bltz`/`bgtz` over `t0` and read back the canonical `bge`/`blt` forms in `disasm`.

Next: [06-memory.md](06-memory.md)
