# easyriscv-term

Bare-metal RISC-V terminal playground inspired by [easy6502](https://github.com/skilldrick/easy6502) (CC BY 4.0).

- RV32I VM in C with ncurses debugger (no stdlib, no Linux syscalls)
- 4KB flat memory, framebuffer at 0x200 (32x32, 16 colors), MMIO at 0xFE/0xFF compat
- Labs build with `clang --target=riscv32 -march=rv32i -nostdlib`

## Quick start

```bash
just build             # build VM
just run 01-pixel      # headless
just debug 01-pixel    # ncurses: SPACE step, r run, g slow, R reset, q quit
just clean             # remove artifacts
```

## Labs

- `01-pixel` - first framebuffer draw (sb to 0x200, 1 byte/pixel)
- `02-fib` - Fibonacci N=7 (branching)
- `03-fib-ram` - Fibonacci vector in RAM 0x300
- `04-branching` - countdown with bne
- `05-memory` - lw/sw/sb/lbu
- `06-stack` - manual sp push/pop
- `07-jumping` - jal/jalr nested calls
- `08-alu` - and/or/xor masking (random at 0xFE)
- `09-snake` - Snake capstone (draft, infinite loop)

## Book

- `book/00-intro.md` - intro and lesson map
- `book/01-numbers.md` - hex and immediates
- `book/02-toolchain.md` - clang/llvm flow
- `book/03-first-pixel.md` - first program
- `book/04-registers.md` - register file
- `book/05-branching.md` - branching
- `book/06-memory.md` - memory
- `book/07-stack.md` - stack
- `book/08-jumping.md` - jumping
- `book/10-io.md` - MMIO and ecall
- `book/11-alu.md` - ALU (needs `10-io`)
- `book/12-fib-ram.md` - Fibonacci vector in RAM

## Solutions

- `solutions/` - exercise solutions per lab,
  layout adapted from [cpantel/Easy6502](https://github.com/cpantel/Easy6502)
  (community solutions for the easy6502 ebook).

Requires: `clang`, `llvm`, `ncursesw`, `just`.
