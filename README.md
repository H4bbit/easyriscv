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

- `01-pixel` - first framebuffer draw
- `02-fib` - Fibonacci N=7
- `03-fib-ram` - Fibonacci with RAM vector at 0x300

Requires: `clang`, `llvm`, `ncursesw`, `just`.
