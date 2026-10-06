# easyriscv-term

[![Open the book](https://img.shields.io/badge/ebook-open-brightgreen)](https://h4bbit.github.io/easyriscv/)

Bare-metal RISC-V terminal playground inspired by [easy6502](https://github.com/skilldrick/easy6502) (CC BY 4.0).

Prefer the browser? Read the [online book](https://h4bbit.github.io/easyriscv/) — same lessons, zero toolchain.

- RV32I VM in C with ncurses debugger (no stdlib, no syscalls — bare metal, MMIO only)
- 4KB flat memory, framebuffer at 0x200 (32x32, 16 colors), MMIO at 0xFE/0xFF compat
- Own assembler in C (`asm.c`, byte-identical to the reference toolchain — `just check-asm` proves it)
- Toolchain-agnostic sources (`#` comments assemble under both clang and GNU `as`)

## Quick start

```bash
just build             # build VM + assembler
just run 01-first-pixel      # headless
just debug 01-first-pixel    # ncurses: SPACE step, r run, g slow, R reset, q quit
just clean             # remove artifacts
```

## Labs (numbered 1:1 with book/ and solutions/)

- `01-first-pixel` - first framebuffer draw (sb to 0x200, 1 byte/pixel)
- `03-registers` - Fibonacci N=7 (branching)
- `04-loop` - countdown with bne (labels + only bne)
- `05-compares` - STUB (beq/blt/bge table)
- `06-bytes-ram` - lw/sw/sb/lbu
- `07-words-vectors` - Fibonacci vector in zero-page RAM 0x00
- `08-dice` - random byte at 0xFE (mask only)
- `09-logic` - and/or/xor masking (needs `08-dice` for `0xFE`)
- `10-big-addresses` - STUB (lui, li unmasked, la)
- `11-stack` - manual sp push/pop
- `12-calls` - jal/jalr nested calls
- `13-keys-print` - STUB (0xFF compares + 0x1000 store)
- `14-snake` - Snake capstone (game loop, eat/grow, self/wall collision, game over)

## Book (1:1 with labs/)

- `book/00-welcome.md` - intro and lesson map
- `book/01-first-pixel.md` - first program
- `book/02-numbers.md` - hex and immediates
- `book/03-registers.md` - register file
- `book/04-loop.md` - loop (labels + only bne)
- `book/05-compares.md` - STUB (compares table)
- `book/06-bytes-ram.md` - bytes in RAM
- `book/07-words-vectors.md` - Fibonacci vector in RAM
- `book/08-dice.md` - random byte at 0xFE (mask only)
- `book/09-logic.md` - ALU (needs `08-dice`)
- `book/10-big-addresses.md` - STUB (lui, li unmasked, la)
- `book/11-stack.md` - stack
- `book/12-calls.md` - calls
- `book/13-keys-print.md` - STUB (0xFF + 0x1000)
- `book/14-snake.md` - capstone glossary (zero new instructions)
- `book/A-toolchain.md` - assembler + reference-toolchain inspection flow
- `book/B-pseudo-table.md` - STUB (canonical↔pseudo table)
- `book/C-memmap.md` - STUB (memory map)

## Solutions

- `solutions/` - exercise solutions per lab,
  layout adapted from [cpantel/Easy6502](https://github.com/cpantel/Easy6502)
  (community solutions for the easy6502 ebook).

## Requires

- C23 (`-std=c23`): `constexpr`, `auto`, `nullptr`, `static_assert`,
  typed `enum`, `[[nodiscard]]` — CI builds clean under Clang 21
  (reference toolchain) and GCC 14 (portability job);
  `CC=`/`CFLAGS=` overrides select the host compiler
- ncurses with wide-char support (`ncursesw`, ABI 6): the debugger uses
  `waddstr` with `▀` half-blocks and 16 color pairs
- Reference RISC-V toolchain for inspection/cross-check (clang+llvm by default:
  `clang --target=riscv32`, `llvm-objcopy`, `llvm-objdump`, `llvm-readelf` —
  `just disasm/elf/hex/check-asm`; `RISCV_CC`/`RISCV_FLAGS`/`OBJCOPY`/`OBJDUMP`/
  `READELF` overrides pick another, e.g. GNU `riscv64-unknown-elf-*` as the
  CI gcc-14 job does) and `just` as task runner
