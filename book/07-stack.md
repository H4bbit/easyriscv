# The Stack

The stack pointer `sp` (`x2`) is just another register — you manage it.

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

Push is `addi sp, -4; sw`, pop is `lw; addi sp, 4`. The stack grows down. With `MEM_SIZE` `0x1000`, `0x900` is a safe top.

The lab pushes `11/22/33` and pops in LIFO order to the framebuffer at `0x200`: it shows `33→11`, `22→6`, `11→11` (low 4 bits). The last word shows `sp & 0xF` back to `0`.

## Try It

Step and watch `sp` in `w_regs` decrement then restore. Forget `addi sp, 4` after `lw` and watch the leak.

## Exercises

1. Push 8 colors and pop to draw a mirrored pattern (first loop pushes, second loop pops and draws).
2. What if you `sw` to `0x1000`? That address is outside the 4KB memory (`0x000-0xFFF`) — the VM intercepts it as a print-char MMIO port instead of RAM.
3. Save/restore `s0` on the stack before a function call (preview of next chapter).

Next: [08-jumping.md](08-jumping.md)
