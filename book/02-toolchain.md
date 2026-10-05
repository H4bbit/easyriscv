# Toolchain

You don't need an IDE. Four commands assemble, run, and inspect bare-metal programs.

## Build

```bash
just build          # cc -std=c23 → vm + asm
```

`just build` compiles `vm.c/ui.c/main.c` (`vm`) and `asm.c` (`asm`)
with `-std=c23` (C23: `constexpr`, `auto`, `nullptr`, `static_assert`)
— the VM links `ncursesw` (wide-char ABI 6: `▀` half-blocks,
16 color pairs). The default is clang (only toolchain tested here);
`CC=`/`CFLAGS=` overrides exist for future compiler testing
(e.g. `CC=gcc just build` — untested path, please report breakage).
No clang-only flags in the default `cflags` on purpose.

## Assemble

```bash
just assemble 01-pixel
```

This runs `./asm` — our own assembler in C (a port of the `riscv.js`
Assembler on the web branch), emitting a flat `.bin` the VM loads at
`0x600`. It is byte-identical to the reference toolchain output for
all labs and solutions (`just check-asm` proves it; the reference is
clang by default, swappable via `RISCV_CC`/`RISCV_FLAGS`/
`OBJCOPY`/`OBJDUMP`/`READELF` for future testing).

The reference toolchain stays as independent ground truth for
inspection only:

```bash
just disasm 01-pixel   # reference-built ELF via objdump (proves decode_to_str)
just hex 01-pixel      # xxd the flat binary
just elf 01-pixel      # llvm-readelf headers and sections
just check-asm         # byte-compare ./asm vs reference on every lab+solution
```

* `./asm` assembles `prog.s` to a flat binary at `0x600`
  (bare-metal, no libc, zero toolchain beyond `cc`)
* `llvm-objdump -d` disassembles the reference-built ELF (clang by default); `xxd` shows the
  flat binary (hexdump); `llvm-readelf -h -S -l` inspects ELF headers
* `--only-section=.text` matters on the reference path: without it the ELF
  headers leak into the first bytes of the flat binary and the VM
  executes garbage

## Run

```bash
just run 01-pixel      # headless: run to halt, print registers and memory
just debug 01-pixel    # ncurses debugger: step, run, inspect
```

Headless prints `Framebuffer 0x200` and zero-page `RAM 0x00`. Debug shows `w_disasm` (yellow PC), `w_regs`, `w_mem` hexdump, `w_fb` pixels.

## Inspect

```bash
just disasm 01-pixel
just hex 01-pixel
just elf 01-pixel
```

These are standard LLVM binutils workflows (`just elf`, `just disasm`).

No hidden magic — what you assemble is what the VM fetches at `pc`.

Next: [03-first-pixel.md](03-first-pixel.md)
