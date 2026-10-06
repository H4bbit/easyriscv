# Memory

> Renamed: old `06-memory` → new `06-bytes-ram`.


RISC-V uses a simple load/store model: arithmetic never touches memory.
Only loads and stores move data between registers and RAM — every access
is `base + 12-bit immediate` (`0(t0)`, `4(t0)`, `12(t0)`). For indexing,
compute the address first: `add t3, t0, t2; sw t1, 0(t3)`.

Run:

```bash
just debug 06-bytes-ram
```

Source (`labs/06-bytes-ram/prog.s`):

```asm
    li t0, 0x00
    sw t1, 0(t0)      # *0x00 = 42
    sw t1, 4(t0)      # *0x004 = 99
    add t3, t0, t2
    sw t1, 0(t3)      # *0x008 = 77 (base+offset)
    lw t4, 0(t0)      # load word
    sb t1, 12(t0)     # byte store
    lbu s1, 12(t0)    # byte load unsigned
```

## Load/Store Only

The canonical forms are `lw` (load word) and `sw` (store word): 4 bytes
at a time, address = register + 12-bit offset. Narrower widths reuse the
same shape with their own funct3:

* `lw`/`sw` — word (4 bytes)
* `lh`/`lhu` — halfword (2 bytes)
* `lb`/`lbu` — byte (sign/zero extended)
* `sb`/`sh` — byte/halfword store

Our memory is `0x000-0xFFF`: `0x000` zero-page RAM, `0x100-0x1FF` stack (top `0x1FC`, grows down), `0x200-0x5FF` framebuffer, `0x600` code.

`just disasm 06-bytes-ram` is the proof: every source line folds to one
encoding, printed with objdump's names — `li t1, 42` at `0x604` is
`addi`, `lbu s1, 12(t0)` at `0x640` is the `lbu` funct3 (`00c2c483`
vs the `lw` funct3 `0002ae83` at `0x624`). No spelling hides a second
instruction.

## Try It

Step and watch `w_mem` hexdump at `0x00`: `42 99 77 171`. The framebuffer at `0x200` shows `13 11` (`141 & 0xF`, `0xAB & 0xF`).

## Translation exercise

Translate both ways and confirm `disasm` does not change: `lw t4, 0(t0)`
loads 4 bytes — spell the 1-byte version (`lb`/`lbu t4, 12(t0)`) and watch
only the funct3 field move; `lbu s1, 12(t0)` ↔ `lb s1, 12(t0)` differ
only in sign- vs zero-extension of the same byte `0xAB` (exercise 1
shows the words `4294967211` vs `171`).

## Exercises

1. Replace `lw t4, 0(t0)` with `lb` vs `lbu`: load the byte `0xAB` at `0x00C` with each. How does sign-extend (`lb`) differ from zero-extend (`lbu`)? ([solution](../solutions/06-bytes-ram/ex01-lb-lbu.s))
2. Use `slli t4, t2, 2` to scale an index by 4 (preview: shifting is covered in [09-logic](09-logic.md); `07-words-vectors` uses `slli t4, t2, 2`) then `add` + `sw` for word-indexed access. ([solution](../solutions/06-bytes-ram/ex02-scaled-index.s))
3. Write a loop that copies 4 words from `0x00` to `0x200`. ([solution](../solutions/06-bytes-ram/ex03-copy-loop.s))

Next: [11-stack.md](11-stack.md)
