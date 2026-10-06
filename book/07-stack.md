# The Stack

The stack pointer `sp` (`x2`) is just another register — the hardware gives
you no push or pop instruction. Push is two canonical instructions
(`addi sp, sp, -4` reserves the slot, `sw t0, 0(sp)` fills it); pop is the
mirror (`lw t0, 0(sp)` reads, `addi sp, sp, 4` releases). The stack grows
down, and forgetting either half corrupts it — your code pushes, your code
pops (the same convention `08-jumping` uses for `ra`).

Run:

```bash
just debug 06-stack
```

Source (`labs/06-stack/prog.s`):

```asm
    li sp, 0x1FC
    addi sp, sp, -4
    sw t0, 0(sp)   # push 11
    lw t0, 0(sp)   # pop
    addi sp, sp, 4
```

Push is `addi sp, -4; sw`, pop is `lw; addi sp, 4`. The stack grows down. With `MEM_SIZE` `0x1000`, `0x1FC` is the stack top (code lives far above at `0x600`).

`just disasm 06-stack` is the proof: no aliases anywhere — `addi sp, sp, -4`
at `0x608` is a plain `addi` (`ffc10113`), `sw t0, 0(sp)` at `0x60c` a plain
`sw` (`00512023`). What the lab calls "push" is two hardware instructions
back to back; the name lives only in the `#` comment.

## Try It

```
[halt] pc=0x0658 t0=12 FB[0..3] = 1 6 11 12, sp=0x1FC
```

Pops come out in LIFO order: `33`, `22`, `11` — masked to low nibbles
(`33&0xF=1`, `22&0xF=6`, `11&0xF=11`). The last byte is `sp & 0xF` (`0x1FC` restored, so `12`).

Step and watch `sp` in `w_regs` decrement then restore. Forget `addi sp, 4` after `lw` and watch the leak.

## Translation exercise

There is no pseudo to translate to here — that is the point. Spell the
push both ways and confirm `disasm` is identical: the two-instruction
`addi sp, sp, -4; sw t0, 0(sp)` vs the same two lines with the `# push`
comment removed. Then break it: drop the `addi sp, sp, 4` after the first
`lw` and watch `sp` stay `4` low — every later pop reads the wrong slot
(exercise 1's mirror comes out shifted).

## Exercises

1. Push 8 colors and pop to draw a mirrored pattern (first loop pushes, second loop pops and draws). ([solution](../solutions/07-stack/ex01-mirror.s))
2. What if you `sw` to `0x1000`? That address is outside the 4KB memory (`0x000-0xFFF`) — the VM intercepts it as a print-char MMIO port instead of RAM (preview: ports are covered in [10-io](10-io.md)). ([solution](../solutions/07-stack/ex02-mmio-print.s))
3. Save/restore `s0` on the stack before a function call (preview of next chapter). ([solution](../solutions/07-stack/ex03-save-s0.s))

Next: [08-jumping.md](08-jumping.md)
