# Memory

RISC-V uses a simple load/store model.

Run:

```bash
just debug 05-memory
```

Source (`labs/05-memory/prog.s`):

```asm
    li t0, 0x300
    sw t1, 0(t0)      // *0x300 = 42
    sw t1, 4(t0)      // *0x304 = 99
    add t3, t0, t2
    sw t1, 0(t3)      // *0x308 = 77 (base+offset)
    lw t4, 0(t0)      // load word
    sb t1, 12(t0)     // byte store
    lbu s1, 12(t0)    // byte load unsigned
```

## Load/Store Only

Every memory access is a `lw`/`sw` (or `lh`/`lb`/`lbu`/`sh`/`sb`) with `base + 12-bit immediate`: `0(t0)` or `4(t0)`. For indexing, compute the address first: `add t3, t0, t2; sw t1, 0(t3)`.

* `lw`/`sw` — word (4 bytes)
* `lh`/`lhu` — halfword (2 bytes)
* `lb`/`lbu` — byte (sign/zero extended)
* `sb`/`sh` — byte/halfword store

Our memory is flat `0x000-0xFFF`: `0x000` code, `0x200` framebuffer, `0x300` RAM.

## Try It

Step and watch `w_mem` hexdump at `0x300`: `42 99 77 171`. The framebuffer at `0x200` shows `13 11` (`141 & 0xF`, `0xAB & 0xF`).

## Exercises

1. Replace `lw t4, 0(t0)` with `lbu`. How does zero-extend differ from `lb` sign-extend?
2. Use `slli t4, t2, 2` to scale `t2` by 4 (as in `03-fib-ram`) then `add` + `sw`.
3. Write a loop that copies 4 words from `0x300` to `0x200`.

Next: [06-stack.md](06-stack.md)
