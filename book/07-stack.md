# The Stack

The stack pointer `sp` (`x2`) is just another register — you manage it.

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

## Try It

```
[halt] pc=0x0658 t0=12 FB[0..3] = 1 6 11 12, sp=0x1FC
```

Pops come out in LIFO order: `33`, `22`, `11` — masked to low nibbles
(`33&0xF=1`, `22&0xF=6`, `11&0xF=11`). The last byte is `sp & 0xF` (`0x1FC` restored, so `12`).

Step and watch `sp` in `w_regs` decrement then restore. Forget `addi sp, 4` after `lw` and watch the leak.

## Exercises

1. Push 8 colors and pop to draw a mirrored pattern (first loop pushes, second loop pops and draws). ([solution](../solutions/07-stack/ex01-mirror.s))
2. What if you `sw` to `0x1000`? That address is outside the 4KB memory (`0x000-0xFFF`) — the VM intercepts it as a print-char MMIO port instead of RAM (preview: ports are covered in [10-io](10-io.md)). ([solution](../solutions/07-stack/ex02-mmio-print.s))
3. Save/restore `s0` on the stack before a function call (preview of next chapter). ([solution](../solutions/07-stack/ex03-save-s0.s))

Next: [08-jumping.md](08-jumping.md)
