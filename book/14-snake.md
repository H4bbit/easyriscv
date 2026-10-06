# Snake: the capstone glossary

> Renamed: old `09-snake` → new `14-snake`.


Every pattern in this book appears once more in `labs/14-snake/prog.s`
(250 lines, 181 instructions): game loop, `0xFF` input, `0xFE` dice,
framebuffer drawing. This chapter adds no new instruction — it names
where each old one works hardest, so you can read the game as a review.

Run headless and it walks right into the wall (no input means `0xFF`
stays `0`, direction never changes) and halts red at `game_over`. Steer
it with:

```bash
just debug 14-snake
```

## Where each chapter lives in the game

* `01-first-pixel` — `draw_snake` paints the head white (`sb t3, 0(t2)`
  with `t3 = 1`) and erases the tail with `sb zero, 0(t2)`; `game_over`
  paints the crash red (`sb t2, 0(t1)` with `t2 = 2`). Same `sb`-to-`0x200`
  you wrote first.
* `03-registers` — `li` loads every constant (`li sp, 0x1FC`,
  `li t3, UP`); the copies are `mv`-shaped `addi`s. Count the `li`s in
  the disasm: most are one `addi`, none needs `lui` (all fits 12 bits).
* `04-loop` — `read_keys` compares the key ASCII against four
  constants (`beq t1, t2, key_up` …); `check_snake` loops with
  `bge t3, t2, no_snake`. The disasm prints the zero-against forms as
  `beqz`/`bnez` (`bnez t0, 0x6a4` in `spin`, `bnez t2, 0x74c` for the
  four `illegal` guards) — the alias you learned in `04-loop`.
  Wall checks add `beqz` (`beqz t3, 0x8c0` falls into `game_over` when the
  column mask hits zero).
* `06-bytes-ram` — the whole game state is a zero-page struct at `0x00`
  (`DIR`/`LEN`/`APPLE`/`SEGS`), read with `lw t1, DIR(t0)` and written
  with `sw t1, DIR(t0)`. Pixel math is `base + index×4` from
  `07-words-vectors`: `slli t4, t3, 2; add t4, t4, t0` addresses `seg[i]`.
* `11-stack` — `init` and `check_collision` are non-leaf: they `jal`
  another function, so they save `ra` first (`addi sp, sp, -4`,
  `sw ra, 0(sp)` … `lw ra, 0(sp)`, `addi sp, sp, 4`). Leaf functions
  (`read_keys`, `update_snake`, `draw_snake`, `draw_apple`) need no save — their `ret` returns
  through the `ra` they received.
* `12-calls` — `loop` calls five subroutines with `jal ra, …`
  (`read_keys`, `check_collision`, `update_snake`, `draw_apple`,
  `draw_snake`); every return is `ret` (`jalr x0, 0(ra)`). The final
  `halt: jal x0, halt` is the jump-to-self you met in `02-numbers`.
* `08-dice` — `gen_apple` rolls dice from `0xFE` (`lbu` + `andi t1, t1, 0x1F`
  for `0-31`, `slli t2, t2, 5` for `Y*32`); `read_keys` reads `0xFF` and
  compares against `0x77`/`0x64`/`0x73`/`0x61` (`w`/`d`/`s`/`a`).
* `09-logic` — direction bits share one `AND` test per opposite pair
  (`andi t2, t1, DOWN` rejects reversal); wall checks are `andi` masks
  (`andi t3, t1, 0x1F` for the column) plus signed compares
  (`blt t1, t3, game_over` above row 0, `bge t1, t3, game_over` below
  row 31).
* `07-words-vectors` — `cs_loop` walks `seg[1..LEN-1]` with the same indexed
  store as the Fibonacci vector (`slli t4, t3, 2` for `i*4`, `add` base,
  `lw`/`sw`), only backward from the tail (`us_loop` shifts each entry
  down one slot per frame).

## Try It

Step `just debug 14-snake` and watch `ra` across one `loop` iteration:
five `jal ra, …` overwrite it five times, and each subroutine's `ret`
returns through the latest copy. Then break it: remove the
`sw ra, 0(sp)` / `lw ra, 0(sp)` pair in `init` and watch the game never
reach `loop` — `gen_apple`'s return address overwrote `init`'s, the same
breakage as exercise 1 in [12-calls](12-calls.md).

## Translation exercise

Translate both ways and confirm `disasm` does not change: `jal ra, init`
↔ `jal x1, init`; each `ret` ↔ `jalr x0, 0(ra)` (and the `x1` spelling,
`jalr x0, 0(x1)` — same encoding, ABI name vs number); the `spin`
delay's `bne t0, zero, spin` ↔ `bnez t0, spin`; `halt: jal x0, halt`
↔ `j halt`.

## Exercises

1. Remove the `sw ra`/`lw ra` in `check_collision` instead of `init`. Which
   subroutine's return breaks first, and where does `pc` land?
2. Change the starting direction (`RIGHT` in `init_snake`) to `DOWN`. Where
   does headless play end now — same wall, different pixel?
3. Shrink the arena: reject columns `≥ 16` in `go_right` (mask + compare,
   like the `0x1F` wrap check). What changes in `gen_apple` to match?

Next: you have the whole machine now — replay any chapter through the
game above, or steer it in `just debug 14-snake` and watch your input
become pixels.
