# Our First Program

Let's dive in! Below is a bare-metal RISC-V program that runs on the terminal VM in this repo — the equivalent of the JavaScript 6502 simulator from easy6502.

```bash
just debug 01-pixel   # ncurses: SPACE step, r run, q quit
# or headless: just run 01-pixel
```

Source (`labs/01-pixel/prog.s`):

```asm
    li t0, 1          // white
    li t1, 0x200
    sw t0, 0(t1)      // pixel (0,0)
    li t0, 5
    sw t0, 4(t1)      // pixel (1,0)
    li t0, 8
    sw t0, 8(t1)      // pixel (2,0)
    j .               // halt
```

You should see three colored pixels at the top-left of the framebuffer panel. Headless shows:

```
Framebuffer 0x200 (words): 1 5 8
```

If you see that, the VM is working.

## Stepping Through

Reset and step (`SPACE`). Watch `pc` and `t0` in `w_regs`:

1. `li t0, 1` — loads immediate `1` into `t0`. Like 6502 `LDA #$01`, but RISC-V has 32 32-bit registers, not one accumulator. `t0` is `x5`.
2. `li t1, 0x200` — `t1 = 0x200`. `0x` prefix means hex, like `$` in 6502.
3. `sw t0, 0(t1)` — stores `t0` to memory at `t1+0` (`0x200`). This is how the framebuffer is drawn. `0x200-0x5FF` maps to the 32×32 display. Low 4 bits are the color (`0` black, `1` white, `5` green...), like easy6502's `STA $0200`. In RISC-V you always use `sw` with a base register + offset — no `STA $0200` absolute form.

Step three more times. `t0` changes to `5` then `8`, and two more pixels appear.

Unlike 6502's `A=$01 → $05 → $08`, here you watch `t0` (and `pc` starting at `0x0` instead of `0x0600`).

## Why This Matters

On real hardware video is more complex, but the idea is the same: memory-mapped I/O. Writing to `0x200` is not "RAM" — the VM's `fb` panel interprets it as a pixel. The same trick is used in `03-fib-ram` where `0x300` is plain RAM.

`j .` is an infinite loop (`jal x0, 0`) — the VM halts when `pc` stops moving, like 6502's `BRK`.

## Exercises

1. Change the color of the three pixels. Try `li t0, 2` (red) or `li t0, 0xE` (light blue).
2. Move one pixel to the bottom-right corner. Hint: word per pixel, so offset `127*4` is last pixel. Try `sw t0, 508(t1)`.
3. Add more instructions to draw a diagonal: `sw` at `0`, `36*4`, `72*4`... (32 pixels per row).

Next: [03-registers.md](03-registers.md) — `zero/t0-t6/a0-ra/sp/pc` vs 6502 `A/X/Y/P`.
