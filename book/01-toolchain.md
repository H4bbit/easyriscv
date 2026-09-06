# Toolchain

You don't need an IDE. Four commands do everything `easy6502`'s buttons did.

## Build

```bash
just build          # clang → vm
```

`just build` compiles `vm.c/ui.c/main.c` with `clang -std=c23` and `ncursesw`.

## Assemble

```bash
just assemble 01-pixel
```

This runs:

```bash
clang --target=riscv32 -march=rv32i -nostdlib -Wl,-Ttext=0x0,--image-base=0x0 \
  -o /tmp/01-pixel.elf labs/01-pixel/prog.s
llvm-objcopy -O binary /tmp/01-pixel.elf /tmp/01-pixel.bin
llvm-objdump -d /tmp/01-pixel.elf   # disassembly
xxd /tmp/01-pixel.bin               # hexdump
```

* `clang` assembles `prog.s` to ELF at `0x0` (bare-metal, no libc)
* `llvm-objcopy` strips ELF to a flat binary the VM loads at `0x0`
* `llvm-objdump -d` is the `Hexdump/Disassemble` button
* `llvm-readelf -h -S -l` is `Monitor` for ELF headers

## Run

```bash
just run 01-pixel      # headless (like Run)
just debug 01-pixel    # ncurses debugger (like Debugger + Step)
```

Headless prints `Framebuffer 0x200` and `RAM 0x300`. Debug shows `w_disasm` (yellow PC), `w_regs`, `w_mem` hexdump, `w_fb` pixels.

## Inspect

```bash
just disasm 01-pixel
just hex 01-pixel
just elf 01-pixel
```

These are the same tools from `linux_user_mode` (`just elf`, `just disasm`).

No hidden magic — what you assemble is what the VM fetches at `pc`.

Next: [02-first-pixel.md](02-first-pixel.md)
