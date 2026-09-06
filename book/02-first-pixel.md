# Our First Program

Let's dive in! Below is a bare-metal RISC-V program that runs on the terminal VM in this repo.

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

1. `li t0, 1` — loads immediate `1` into `t0`. `t0` is register `x5`. RISC-V has 32 32-bit registers; `li` is a pseudo-instruction for `addi t0, zero, 1`.
2. `li t1, 0x200` — `t1 = 0x200`. `0x` means hex.
3. `sw t0, 0(t1)` — stores the value in `t0` to memory at address `t1+0` (`0x200`). This is how the framebuffer is drawn. `0x200-0x5FF` maps to the 32×32 display. Low 4 bits are the color (`0` black, `1` white, `5` green, `8` orange...).

Step three more times. `t0` changes to `5` then `8`, and two more pixels appear. `pc` starts at `0x0` and advances by 4 per instruction.

## Why This Matters

On real hardware video is more complex, but the idea is the same: memory-mapped I/O. Writing to `0x200` is not plain RAM — the VM's `fb` panel interprets it as a pixel. Plain RAM is at `0x300` (used in later labs).

`j .` is an infinite loop (`jal x0, 0`) — the VM halts when `pc` stops moving.

## Exercises

1. Change the color of the three pixels. Try `li t0, 2` (red) or `li t0, 0xE` (light blue).
2. Move one pixel to the bottom-right corner. Hint: one word per pixel, so offset `127*4 = 508` is the last pixel. Try `sw t0, 508(t1)`.
3. Add more instructions to draw a diagonal: `sw` at offsets `0`, `36*4`, `72*4`... (32 pixels per row).

Next: [03-registers.md](03-registers.md)
