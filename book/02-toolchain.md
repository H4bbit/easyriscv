# Toolchain

You don't need an IDE. Four commands assemble, run, and inspect bare-metal programs.

## Build

```bash
just build          # cc -std=c23 → vm
```

`just build` compiles `vm.c/ui.c/main.c` with `-std=c23` (C23:
`constexpr`, `auto`, `nullptr`, `static_assert`) linked against
`ncursesw` (wide-char ABI 6: `▀` half-blocks, 16 color pairs).
Set `cc=gcc` (or any C23-capable compiler) to build with another
toolchain — the default is `clang`.

## Assemble

```bash
just assemble 01-pixel
```

This runs:

```bash
clang --target=riscv32 -march=rv32i -nostdlib -Wl,-Ttext=0x600,--image-base=0x600 \
  -o /tmp/01-pixel.elf labs/01-pixel/prog.s
llvm-objcopy -O binary --only-section=.text /tmp/01-pixel.elf /tmp/01-pixel.bin
llvm-objdump -d /tmp/01-pixel.elf   # disassembly
xxd /tmp/01-pixel.bin               # hexdump
```

* `clang` assembles `prog.s` to ELF at `0x600` (bare-metal, no libc)
* `llvm-objcopy` strips ELF to a flat `.text` binary the VM loads at `0x600` (`--only-section=.text` matters: without it the ELF headers leak into the first bytes of the flat binary and the VM executes garbage)
* `llvm-objdump -d` disassembles the ELF; `xxd` shows the flat binary (hexdump)
* `llvm-readelf -h -S -l` inspects ELF headers and sections

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

These are standard LLVM binutils workflows, also used for hosted Linux binaries (`just elf`, `just disasm`).

No hidden magic — what you assemble is what the VM fetches at `pc`.

Next: [03-first-pixel.md](03-first-pixel.md)
