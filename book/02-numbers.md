# Numbers

> Renamed: old `01-numbers` → new `02-numbers`.

Before writing more code, a quick note on numbers. There is no lab of
its own here — every example below is a line you already ran in
`01-first-pixel`.

## Hex

RISC-V assemblers understand decimal and hex. Hex is prefixed with `0x`:

* `42` decimal
* `0x2A` hex — same value, 42
* `0x200` hex = 512 decimal — our framebuffer base
* `0xF` = 15 decimal — max color nibble

If you're not familiar with hex, read the [Wikipedia article](https://en.wikipedia.org/wiki/Hexadecimal). Values like `0x20`, `0xFF` are easier to read as addresses and bit patterns than decimal.

Hex digits are `0-9 A-F`. Each digit is 4 bits, each byte is two digits (`0x00`-`0xFF`), each word is eight digits (`0x00000000`-`0xFFFFFFFF`).

## The color nibble

Each framebuffer byte is one pixel, and only its low 4 bits pick the
color (`0`-`15`). That is exactly one hex digit: `sb` a `0x5` and the
pixel is green, `sb` a `0x8` and it is orange. A value like `0x15`
still shows color `5` — the high nibble is ignored by the `fb` panel.
Reading colors as hex digits (`0x0`-`0xF`) instead of decimal (`0`-`15`)
is the habit from here on.

## Immediates vs offsets (prose only)

In `li t0, 5`, the `5` is an immediate — a value encoded directly in
the instruction that lands in the register. In `sb t0, 0(t1)`, the `0`
is an offset added to the base register `t1` — it locates memory, it
never lands in a register. Same `0x` digits, two jobs: immediates fill
registers, offsets shift addresses.

`just disasm 01-first-pixel` is the proof you can peek at (not homework):

```
     608: 00530023	sb	t0, 0(t1)
```

One line is enough for now: the offset `0` lives in its own field of
the `sb` encoding, separate from the immediates in the `li` lines above
it. How `li` packs its immediate stays magic until chapter `10`.

No pseudo is taught here — that is the point, and there is no
translation exercise yet.

## Words

All RISC-V registers are 32 bits (4 bytes). `pc` advances by 4. Our
memory holds words at `0x000, 0x004, 0x008...` but `sb` still stores a
single byte at any address — that is how one pixel lands without
touching its neighbors.

## Try It

```bash
just debug 01-first-pixel   # watch t0 = 0x1, 0x5, 0x8 (hex, not decimal)
```

Step the three `li` lines and read `t0` in hex: `0x1` white, `0x5`
green, `0x8` orange. Then step each `sb` and watch the matching pixel
take the low nibble.

## Exercises

1. What is `0x10` in decimal? `0xFF`? `0x200`?
2. If `t0` holds `0x1B`, what color does `sb t0, 0(t1)` draw — and why?
3. In `labs/01-first-pixel/prog.s`, point at each number and name its
   job: immediate (fills a register) or offset (shifts an address).

Next: [03-registers.md](03-registers.md)
