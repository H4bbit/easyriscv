# Numbers and Immediates

Before writing code, a quick note on numbers.

## Hex

RISC-V assemblers understand decimal and hex. Hex is prefixed with `0x`:

* `42` decimal
* `0x2A` hex — same value, 42
* `0x200` hex = 512 decimal — our framebuffer base
* `0xF` = 15 decimal — max color nibble

If you're not familiar with hex, read the [Wikipedia article](https://en.wikipedia.org/wiki/Hexadecimal). Values like `0x20`, `0xFF` are easier to read as addresses and bit patterns than decimal.

Hex digits are `0-9 A-F`. Each digit is 4 bits, each byte is two digits (`0x00`-`0xFF`), each word is eight digits (`0x00000000`-`0xFFFFFFFF`).

## Immediates vs Memory

In `li t0, 5`, the `5` is an immediate — a value encoded directly in the instruction. In `sw t0, 0(t1)`, the `0` is an offset added to the base register `t1`. This book uses `0x` for immediates and `0(t1)` for memory offsets.

`li` is a pseudo-instruction. For small values (`-2048` to `2047`) it's just one instruction: `addi t0, zero, imm` — `li t0, 0x200` in `01-pixel` is exactly that (`0x200` = 512 fits 12 bits, `just disasm 01-pixel` proves it). Larger values need two: `lui t0, hi` + `addi t0, t0, lo` — `li t0, 0x12345` below assembles to `lui t0, 0x12` + `addi t0, t0, 0x345` (`just disasm` shows both; the hardware never sees `li`). This is the central reference for the table: every later chapter's `li` folds into one of these two shapes.

## Words

All RISC-V registers are 32 bits (4 bytes). `pc` advances by 4. Our memory is words at `0x000, 0x004, 0x008...` but we can still store bytes with `sb`.

## Try It

```bash
just assemble 01-pixel
# check disassembly: li t1, 0x200 is a single addi (512 fits 12 bits)
just debug 01-pixel   # watch t0 = 0x1, 0x5, 0x8
```

## Exercises

1. What is `0x10` in decimal? `0xFF`?
2. Assemble `li t0, 0x12345` and run `just disasm` on it — you should see `lui t0, 0x12` followed by `addi t0, t0, 0x345`: two instructions, because `0x12345` does not fit 12 bits. Contrast with `li t1, 0x200`, which stays one `addi`.
3. Change `sw t0, 0(t1)` to `sw t0, 1(t1)` — what happens to alignment? (Hint: `lw` requires aligned)

Next: [02-toolchain.md](02-toolchain.md)
