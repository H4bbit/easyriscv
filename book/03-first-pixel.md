# Our First Program

Let's dive in! Below is a bare-metal RISC-V program that runs on the terminal VM in this repo.

```bash
just debug 01-pixel   # ncurses: SPACE step, r run, q quit
# or headless: just run 01-pixel
```

Source (`labs/01-pixel/prog.s`) — step 1 is canonical, step 2 is shorthand:

```asm
    addi t0, zero, 1      # canonical: t0 = 1 (white)
    addi t1, zero, 0x200 # canonical: t1 = 0x200
    sb t0, 0(t1)      # pixel (0,0) - 1 byte per pixel
    li t0, 5          # shorthand for addi t0, zero, 5: green
    sb t0, 1(t1)      # pixel (1,0)
    li t0, 8          # shorthand for addi t0, zero, 8: orange
    sb t0, 2(t1)      # pixel (2,0)
    jal x0, .         # halt: jump to self (j . is shorthand)
```

`addi t0, zero, 1` adds `1` to the always-zero register — that is how a
constant enters a register. `li t0, 5` is the assembler's shorthand for the
same shape (`addi t0, zero, 5`); `just disasm 01-pixel` shows both forms
as `addi`. `jal x0, .` jumps to itself: `pc` never moves, so the VM halts
(`j .` is the shorthand).

You should see three colored pixels at the top-left of the framebuffer panel. Headless shows:

```
Framebuffer 0x200 (bytes, 1 byte/pixel): 1 5 8
```

Palette (16 colors, nibble `0`-`15`): `0` black, `1` white, `2` red, `3` cyan, `4` purple, `5` green, `6` blue, `7` yellow, `8` orange... `15` light grey.

If you see that, the VM is working.

## Stepping Through

Reset and step (`SPACE`). Watch `pc` and `t0` in `w_regs`:

1. `addi t0, zero, 1` — loads `1` into `t0` (`x5`). RISC-V has 32 32-bit registers; adding to `zero` is the canonical way to load a small constant.
2. `addi t1, zero, 0x200` — `t1 = 0x200`. `0x` means hex.
3. `sb t0, 0(t1)` — stores the low byte of `t0` to memory at `t1+0` (`0x200`). This is how the framebuffer is drawn. `0x200-0x2FF` is row 0, `0x220-` row 1, etc. — 32 bytes per row, 32 rows. Low 4 bits are the color.

Then the same idea in shorthand: `li t0, 5` / `li t0, 8` (each is `addi` from `zero`). Step them and watch `t0` change to `5` then `8` with two more pixels. `pc` starts at `0x600` and advances by 4 per instruction.

## Why This Matters

On real hardware video is more complex, but the idea is the same: memory-mapped I/O. Writing to `0x200` is not plain RAM — the VM's `fb` panel interprets it as a pixel. Plain RAM is the zero page at `0x00` (used in later labs).

`jal x0, .` is an infinite loop (`jal` with `x0` discards the return address) — the VM halts when `pc` stops moving.

## Exercises

1. Change the color of the three pixels. Try `li t0, 2` (red) or `li t0, 0xE` (light blue). ([solution](../solutions/01-pixel/ex01-change-color.s))
2. Move one pixel to the bottom-right corner. Hint: 1 byte per pixel, offset `1023` is the last pixel (`31,31`). Try `sb t0, 1023(t1)` or `sb t0, 31(t1)` for row 0 end. ([solution](../solutions/01-pixel/ex02-bottom-right.s))
3. Add more instructions to draw a diagonal: `sb` at offsets `0`, `33`, `66`... (32 bytes per row, +33 = down+right). ([solution](../solutions/01-pixel/ex03-diagonal.s))

Next: [04-registers.md](04-registers.md)
