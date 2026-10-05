# Fibonacci in RAM

`02-fib` computed Fibonacci(7) = 13 but kept only the last two values.
What if you need the whole sequence? Store it — this lab turns registers
into a vector, the pattern every array program reuses.

## Setup: `02-fib` recap

`labs/02-fib/prog.s` is the same loop without stores:

```asm
    li a0, 7
    li t0, 0          // F(0)
    li t1, 1          // F(1)
loop:
    add t3, t0, t1
    mv  t0, t1
    mv  t1, t3
    blt t2, a0, loop  // until counter == 7
```

Result `a0=13`, nothing in RAM. If you haven't traced it in
`04-registers`, do that first.

Run:

```bash
just debug 03-fib-ram
```

Source (`labs/03-fib-ram/prog.s`):

```asm
    li s0, 0x00       // zero-page RAM base (code lives at 0x600, far away)
    sw t0, 0(s0)      // RAM[0x00] = 0
    sw t1, 4(s0)      // RAM[0x004] = 1
loop:
    add t3, t0, t1
    slli t4, t2, 2    // counter * 4 (words are 4 bytes)
    add  t5, s0, t4
    sw   t3, 0(t5)    // RAM[s0 + counter*4] = t3
    blt  t2, a0, loop
```

## Base + index×4

Word arrays need scaling: element `i` lives at `base + i*4`. Three
instructions do it — shift, add, store:

1. `slli t4, t2, 2` — `i × 4` (<< 2 is the ×4 you met in [11-alu](11-alu.md))
2. `add t5, s0, t4` — address = base + offset
3. `sw t3, 0(t5)` — store the word

The base `0x00` is zero-page RAM, far below the code at `0x600` —
no address juggling needed (the old `0x0`-linked layout forced RAM to
dodge the code; that architectural mistake is fixed)..

## Try It

Step and watch `w_mem` / headless `RAM 0x00`: `0 1 1 2 3 5 8 13`.
Each loop iteration appends one word. Final `a0=13`, same as `02-fib` —
the value is identical, the storage is new.

## Exercises

1. Change `N` to `10`: what are `a0` and the last vector word? ([solution](../solutions/12-fib-ram/ex01-n10.s))
2. Store bytes instead: `sb` the low byte of each term — what does the vector look like? ([solution](../solutions/12-fib-ram/ex02-bytes.s))
3. Sum the vector back: loop `i=0..7`, `lw` each word, accumulate in `a1`. ([solution](../solutions/12-fib-ram/ex03-sum.s))

Next: the Snake capstone (`labs/09-snake`, WIP) will reuse every pattern in this book: game loop, `0xFF` input, `0xFE` dice, framebuffer drawing.
