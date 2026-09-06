# The Stack

6502's stack lives at `$0100-$01FF` with automatic `SP`. RISC-V's `sp` (`x2`) is just another register — you manage it.

Run:

```bash
just debug 06-stack
```

Source (`labs/06-stack/prog.s`):

```asm
    li sp, 0x900
    addi sp, sp, -4
    sw t0, 0(sp)   // push 11
    lw t0, 0(sp)   // pop
    addi sp, sp, 4
```

Push is `addi sp, -4; sw`, pop is `lw; addi sp, 4`. Stack grows down. Our `MEM_SIZE` is `0x1000`, so `0x900` is safe top — like 6502's `$01FF`.

The lab pushes `11/22/33` and pops to `FB 0x200` in LIFO order: `33→11`, `22→6`, `11→11` (low 4 bits). `FB[3]` shows `sp & 0xF` back to `0`.

## Try It

Step and watch `sp` in `w_regs` decrement then restore. Forget `addi sp,4` after `lw` and watch the leak — like forgetting `PLA`.

## Exercises

1. Push 8 colors and pop to draw mirrored pattern (easy6502's `PHA/PLA` example).
2. What if you `sw` to `0x1000`? That's out-of-bounds — now MMIO (see `kernel.s`).
3. Save/restore `s0` on stack before a function call (preview).

Next: [07-jumping.md](07-jumping.md) — `jal/jalr` vs `JMP/JSR/RTS`.
