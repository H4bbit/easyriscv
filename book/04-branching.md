# Branching

So far we've run straight-line code. Now let's make it loop.

Run the lab:

```bash
just debug 04-branching
```

Source (`labs/04-branching/prog.s`):

```asm
    li t0, 8
    li t1, 0x200
    li t2, 3
loop:
    addi t0, t0, -1
    sw   t0, 0(t1)
    bne  t0, t2, loop   // branch if t0 != t2
```

## How Branches Work

`bne t0, t2, loop` means `if (t0 != t2) pc = loop`. No separate compare instruction is needed — the branch does the compare itself. Other forms: `beq` (equal), `blt` (signed less), `bge`, `bltu` (unsigned), `bgeu`. Pseudo `beqz rs, label` is `beq rs, zero, label`.

The branch offset is a 13-bit PC-relative immediate assembled from the label.

## Try It

Step through `04-branching`. `t0` goes `8→7→6→5→4→3` then falls through. The framebuffer at `0x200` shows `3` (low 4 bits of the last store).

## Exercises

1. Replace `bne` with `beq`. What happens?
2. Use `blt t0, t2, loop` — how does signed less-than differ?
3. Write a loop that counts up from `0` to `5` using `addi` and `bne`.

Next: [05-memory.md](05-memory.md)
