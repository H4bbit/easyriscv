# easyriscv-term

Bare-metal RISC-V playground: a tiny RV32I VM with a terminal debugger,
a 32x32 framebuffer, and self-contained lessons.

## What this is

* `vm.c` — RV32I CPU: fetch/decode/execute, 4KB flat memory, framebuffer at
  `0x200` (32x32, 16 colors), random byte at `0xFE`, last key at `0xFF`,
  print-char port at `0x1000`
* `ui.c` — ncurses debugger: disassembly (PC in yellow), registers,
  memory hexdump, framebuffer pixels
* `labs/` — one `prog.s` per lesson, assembled bare-metal at `0x600` (code above the screen, state below it)
* `book/` — self-contained lessons, `00` to `14` plus appendices `A`–`C`, no prerequisites
* `solutions/` — exercise answers per lab

## Quick start

```bash
just build             # build VM
just run 01-first-pixel      # headless
just debug 01-first-pixel    # ncurses: SPACE step, r run, g slow, R reset, q quit
just clean             # remove artifacts
```

## Lessons (`book/NN == labs/NN == solutions/NN`)

| Lab | Book | New family only | Pseudo budget |
|-----|------|-----------------|---------------|
| — | `00-welcome` | machine table + how to run, NO code | none |
| `01-first-pixel` | `01-first-pixel` | `sb`, `j .` halt; `li` is MAGIC (no expansion yet) | none taught (`li` used blind) |
| — | `02-numbers` | hex + color nibble, immediates vs offsets (prose only) | none |
| `03-registers` | `03-registers` | `zero`/`t0`/`pc`, `addi`/`add` | `mv` ↔ `addi rd,rs,0` |
| `04-loop` | `04-loop` | labels + **only `bne`** | none (`beq` is an exercise) |
| `05-compares` | `05-compares` | `beq`/`blt`/`bge`/`bltu`/`bgeu` table | `beqz`/`bnez` only; rest is lookup table |
| `06-bytes-ram` | `06-bytes-ram` | `sb`/`lb`/`lbu`, zero-page `0x00`, `base+offset` | none |
| `07-words-vectors` | `07-words-vectors` | `lw`/`sw`, `slli` ×4, shift→add→store pattern | `mv` reuse only, nothing new |
| `08-dice` | `08-dice` | `0xFE` random port | `andi` as mask ONLY |
| `09-logic` | `09-logic` | `and`/`or`/`xor` + `i`-forms | `not` ↔ `xori -1`, `neg` ↔ `sub x0` |
| `10-big-addresses` | `10-big-addresses` | `lui`, `li` unmasked, `slli`/`srli`/`srai`, `la` (near only) | `li` itself + central pseudo table HERE |
| `11-stack` | `11-stack` | `sp=0x1FC`, push=`addi`+`sw`, pop=`lw`+`addi` | none |
| `12-calls` | `12-calls` | `jal`/`jalr`/`ret`/`j`, leaf vs non-leaf | `ret` ↔ `jalr x0,0(x1)`; `call`/`tail` = 3-line note |
| `13-keys-print` | `13-keys-print` | `0xFF` compares + `0x1000` store | none new |
| `14-snake` | `14-snake` | GLOSSARY, zero new instructions | none |
| — | `A-toolchain` | (appendix, slimmed, never blocking the framebuffer) | — |
| — | `B-pseudo-table` | (appendix: canonical↔pseudo table) | — |
| — | `C-memmap` | (appendix: memory map) | — |

Capstone: `14-snake` (game loop, `WASD` at `0xFF`, apple at random,
state in zero-page `0x00`, full 32x32 arena playable).
Headless without input walks right into the wall and halts red at `game_over`;
press keys in `just debug 14-snake` to steer and eat (grows `LEN`).

## Solutions

* `solutions/` — exercise solutions per lab

Requires: `clang`, `llvm`, `ncursesw`, `just`.
