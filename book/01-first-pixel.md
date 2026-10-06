# Our First Program

> Renamed: old `03-first-pixel` → new `01-first-pixel` (1:1 `book/01 == labs/01 == solutions/01`).

Let's dive in! Below is a bare-metal RISC-V program that runs on the terminal VM in this repo.

```bash
just debug 01-first-pixel   # ncurses: SPACE step, r run, q quit
# or headless: just run 01-first-pixel
```

Source (`labs/01-first-pixel/prog.s`) — three ideas, nothing else:

```asm
    li t0, 1          # white (MAGIC for now: how it works is chapter 10)
    li t1, 0x200      # framebuffer base
    sb t0, 0(t1)      # pixel (0,0) - 1 byte per pixel
    li t0, 5          # green
    sb t0, 1(t1)      # pixel (1,0)
    li t0, 8          # orange
    sb t0, 2(t1)      # pixel (2,0)
    j .               # halt: jump to self, pc stops
```

Three ideas, each one line:

1. `li t0, 1` puts the constant `1` in register `t0`. HOW the assembler
   does it is chapter `10`'s job — today `li` is magic: it just works
   for small values like `1`, `5`, `8`, `0x200`. (`0x` means hex;
   `0x200` = 512, the framebuffer base.)
2. `sb t0, 0(t1)` stores the low byte of `t0` to memory at `t1+0`
   (`0x200`). That address is not plain RAM — the VM's `fb` panel
   interprets it as pixel `(0,0)`. `0x200`-`0x2FF` is row 0,
   `0x220`- is row 1, and so on: 32 bytes per row, 32 rows, 1 byte per
   pixel, low 4 bits are the color.
3. `j .` jumps to itself (`.` = this address). `pc` never moves, so the
   VM halts. Every lab in this book ends this way.

No pseudo is taught here — that is the point. `li` is used blind and
unmasked in chapter `10`; there is no translation exercise yet.

`just disasm 01-first-pixel` is the proof you can peek at (not homework):

```
     61c: 0000006f     	j\t0x61c
```

One line is enough for now: `j .` folds to a single `jal x0, 0` that
the decoder prints as `j`. Each `li` prints as `li`, each `sb` as `sb`.
The hardware folds every spelling to one encoding — from chapter `03`
on you will read both sides with `disasm` as the answer key.

You should see three colored pixels at the top-left of the framebuffer panel. Headless shows:

```
Framebuffer 0x200 (bytes, 1 byte/pixel): 1 5 8
```

Palette (16 colors, nibble `0`-`15`): `0` black, `1` white, `2` red, `3` cyan, `4` purple, `5` green, `6` blue, `7` yellow, `8` orange... `15` light grey.

If you see that, the VM is working.

## Stepping Through

Reset and step (`SPACE`). Watch `pc` and `t0` in `w_regs`:

1. `li t0, 1` — `t0` becomes `1`. RISC-V has 32 32-bit registers;
   `t0` is a scratch temporary (`x5`).
2. `li t1, 0x200` — `t1` becomes `0x200`, the framebuffer base.
3. `sb t0, 0(t1)` — pixel `(0,0)` lights up white.

Then the same idea twice more: `li t0, 5` / `sb t0, 1(t1)` (green at
`(1,0)`), `li t0, 8` / `sb t0, 2(t1)` (orange at `(2,0)`), then the
halt. `pc` starts at `0x600` and advances by 4 per instruction.

## Why This Matters

On real hardware video is more complex, but the idea is the same: memory-mapped I/O. Writing to `0x200` is not plain RAM — the VM's `fb` panel interprets it as a pixel. Plain RAM is the zero page at `0x00` (used in later labs).

## Exercises

1. Change the color of the three pixels. Try `li t0, 2` (red) or `li t0, 0xE` (light blue). ([solution](../solutions/01-first-pixel/ex01-change-color.s))
2. Move one pixel to the bottom-right corner. Hint: 1 byte per pixel, offset `1023` is the last pixel (`31,31`). Try `sb t0, 1023(t1)` or `sb t0, 31(t1)` for row 0 end. ([solution](../solutions/01-first-pixel/ex02-bottom-right.s))
3. Add more instructions to draw a diagonal: `sb` at offsets `0`, `33`, `66`... (32 bytes per row, +33 = down+right). ([solution](../solutions/01-first-pixel/ex03-diagonal.s))

Next: [02-numbers.md](02-numbers.md)
