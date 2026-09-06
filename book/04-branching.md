# Branching

So far we've run straight-line code. Now let's loop and branch — the RISC-V way vs 6502's flag-based branches.

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

This mirrors easy6502:

```asm
  LDX #$08
decrement:
  DEX
  STX $0200
  CPX #$03
  BNE decrement
```

## Flags vs Explicit Compare

6502's `CPX` sets the `Z` flag, then `BNE` tests `Z==0`. RISC-V has no flags. `bne t0, t2, loop` directly compares two registers — `if (t0 != t2) pc = loop`. Other branches: `beq` (equal), `blt` (signed less), `bge`, `bltu` (unsigned), `bgeu`. And pseudo `beqz rs, label` is `beq rs, zero, label`.

The immediate is a 13-bit PC-relative offset (like 6502's 8-bit relative, but larger). Labels become offsets at assemble time.

## Try It

Step through `04-branching`. `t0` goes `8→7→6→5→4→3` then falls through. `FB 0x200` shows `3` (low 4 bits of last store).

## Exercises

1. Replace `bne` with `beq`. What happens? (Hint: loops only if equal on first iteration — it doesn't.)
2. Use `blt t0, t2, loop` vs `bne`. How does signed compare differ?
3. Write a `beqz` loop that counts up from `0` to `5` using `addi` and `beqz`? Actually you need `bne` — try both.

Next: [05-memory.md](05-memory.md) — `lw/sw` vs 6502 addressing modes.
