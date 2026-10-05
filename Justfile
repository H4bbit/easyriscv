set shell := ["bash", "-cu"]

vm := "vm"
# Portable scratch dir for .elf/.bin: honors $TMPDIR (Termux, macOS,
# Nix) with /tmp fallback. Never hardcode an OS-specific tmp path here.
bindir := env_var_or_default("TMPDIR", "/tmp")
# Compiler override: `cc=gcc just build` (default clang). Any C23-capable
# compiler works; see README "Requires" for the C23/ncursesw versions.
# (No RISCV_CC: labs assemble with our own ./asm in C; clang stays as
# independent ground truth for inspection — just disasm/elf/hex/check-asm.)
cc := env_var_or_default("CC", "clang")
cflags := "-std=c23 -O2 -Wall -Wextra -Wpedantic -Wshadow -Wundef -Werror=implicit-function-declaration -Werror=return-type -fdiagnostics-color=always -fcolor-diagnostics -I."
ldflags := "-lncursesw"

# Build VM (replaces Makefile)
build:
    {{cc}} {{cflags}} -c vm.c -o vm.o
    {{cc}} {{cflags}} -c ui.c -o ui.o
    {{cc}} {{cflags}} -c main.c -o main.o
    {{cc}} {{cflags}} -c asm.c -o asm.o
    {{cc}} -o {{vm}} vm.o ui.o main.o {{ldflags}}
    {{cc}} {{cflags}} -o asm asm.o

clean:
    rm -f vm.o ui.o main.o asm.o {{vm}} asm {{bindir}}/*.elf {{bindir}}/*.bin

# Assemble lab: prog.s -> bin via our own ./asm (clang-identical, see just check-asm).
# clang stays as independent ground truth for inspection (disasm/elf/hex).
assemble lab="01-pixel":
    #!/bin/bash
    set -e
    src="labs/{{lab}}/prog.s"
    bin="{{bindir}}/{{lab}}.bin"
    echo "==> {{lab}}: $src"
    [ -x ./asm ] || just build
    ./asm $src $bin
    echo "BIN:"; xxd $bin | head -n 5
    echo "size $(wc -c < $bin) bytes"

run lab="01-pixel": (assemble lab)
    ./{{vm}} {{bindir}}/{{lab}}.bin --headless

debug lab="01-pixel": (assemble lab)
    ./{{vm}} {{bindir}}/{{lab}}.bin

# Run all labs headless (09-snake halts at game_over without input; steer with just debug 09-snake. 08-alu/10-io use random)
all:
    just run 01-pixel
    just run 02-fib
    just run 03-fib-ram
    just run 04-branching
    just run 05-memory
    just run 06-stack
    just run 07-jumping
    just run 10-io

# Run with step limit (for infinite loops like Snake)
run-steps lab="09-snake" steps="1000":
    just assemble {{lab}}
    ./vm {{bindir}}/{{lab}}.bin --headless --steps {{steps}}

# Inspection (independent clang ground truth: needs llvm binutils)
# Build the reference ELF via clang, proving decode_to_str against llvm-objdump.
disasm lab="01-pixel":
    clang --target=riscv32 -march=rv32i -nostdlib -Wl,-Ttext=0x600,--image-base=0x600 -o {{bindir}}/{{lab}}.elf labs/{{lab}}/prog.s
    llvm-objdump -d {{bindir}}/{{lab}}.elf

elf lab="01-pixel":
    clang --target=riscv32 -march=rv32i -nostdlib -Wl,-Ttext=0x600,--image-base=0x600 -o {{bindir}}/{{lab}}.elf labs/{{lab}}/prog.s
    llvm-readelf -h -S -l {{bindir}}/{{lab}}.elf

hex lab="01-pixel":
    xxd {{bindir}}/{{lab}}.bin

# Cross-check: our ./asm must be byte-identical to clang for every lab
# and every solution. Catches drift between asm.c and the toolchain.
check-asm:
    #!/bin/bash
    set -e
    [ -x ./asm ] || just build
    fail=0
    for src in labs/*/prog.s solutions/*/*.s; do \
      clang --target=riscv32 -march=rv32i -nostdlib -Wl,-Ttext=0x600,--image-base=0x600 -o "{{bindir}}/ref.elf" "$src" 2>/dev/null; \
      llvm-objcopy -O binary --only-section=.text "{{bindir}}/ref.elf" "{{bindir}}/ref.bin"; \
      ./asm "$src" "{{bindir}}/my.bin"; \
      cmp -s "{{bindir}}/ref.bin" "{{bindir}}/my.bin" || { echo "DIFF $src"; fail=1; }; \
    done
    rm -f "{{bindir}}/ref.elf" "{{bindir}}/ref.bin" "{{bindir}}/my.bin"
    [ "$fail" = 0 ] && echo "check-asm: all labs+solutions byte-identical" || exit 1
