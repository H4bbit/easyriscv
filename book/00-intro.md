# easyriscv-term

Bare-metal RISC-V playground: a tiny RV32I VM with a terminal debugger,
a 32x32 framebuffer, and self-contained lessons.

## What this is

* `vm.c` — RV32I CPU: fetch/decode/execute, 4KB flat memory, framebuffer at
  `0x200` (32x32, 16 colors), random byte at `0xFE`, last key at `0xFF`,
  print-char port at `0x1000`
* `ui.c` — ncurses debugger: disassembly (PC in yellow), registers,
  memory hexdump, framebuffer pixels
* `labs/` — one `prog.s` per lesson, assembled bare-metal at `0x0`
* `book/` — self-contained lessons, `00` to `12`, no prerequisites
* `solutions/` — exercise answers per lab

## Quick start

```bash
just build             # build VM
just run 01-pixel      # headless
just debug 01-pixel    # ncurses: SPACE step, r run, g slow, R reset, q quit
just clean             # remove artifacts
```

## Lessons

| Lab | Book | Topic |
|-----|------|-------|
| — | `01-numbers` | hex and immediates (`lui`/`addi`, `li` pseudo-op) |
| — | `02-toolchain` | clang/llvm flow (`assemble/run/debug/disasm/hex/elf`) |
| `01-pixel` | `03-first-pixel` | first framebuffer draw (`sb` to `0x200`, 1 byte/pixel) |
| `02-fib` | `04-registers` | register file (`t0/t1/a0`, 3-operand `add`) |
| `04-branching` | `05-branching` | countdown with `bne` (explicit compare, no flags) |
| `05-memory` | `06-memory` | `lw`/`sw`/`sb`/`lbu`, base+offset |
| `06-stack` | `07-stack` | manual `sp` push/pop |
| `07-jumping` | `08-jumping` | `jal`/`jalr` nested calls |
| `10-io` | `10-io` | MMIO ports (`0xFE` random, `0xFF` key, `0x1000` print) |
| `08-alu` | `11-alu` | `and`/`or`/`xor`, shifts, `sub`/`slt` (needs `10-io` for `0xFE`) |
| `03-fib-ram` | `12-fib-ram` | vector in RAM `0x300` (`slli` index×4) |

Capstone (WIP): `09-snake` (game loop, `WASD` at `0xFF`, apple at random).

## Solutions

* `solutions/` — exercise solutions per lab

Requires: `clang`, `llvm`, `ncursesw`, `just`.
