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

- `book/00-intro.md` - intro
- `book/01-numbers.md` - hex and immediates
- `book/01-toolchain.md` - clang/llvm flow
- `book/02-first-pixel.md` - first program
- `book/03-registers.md` - register file
- `book/04-branching.md` - branching
- `book/05-memory.md` - memory
- `book/06-stack.md` - stack
- `book/07-jumping.md` - jumping

## Solutions

- `solutions/` - exercise solutions per lab (01-pixel, 04-branching, 05-memory)

Requires: `clang`, `llvm`, `ncursesw`, `just`.
