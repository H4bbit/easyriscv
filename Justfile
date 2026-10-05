set shell := ["bash", "-cu"]

vm := "vm"
# Portable scratch dir for .elf/.bin: honors $TMPDIR (Termux, macOS,
# Nix) with /tmp fallback. Never hardcode an OS-specific tmp path here.
bindir := env_var_or_default("TMPDIR", "/tmp")
# Compiler override: `cc=gcc just build` (default clang). Any C23-capable
# compiler works; see README "Requires" for the C23/ncursesw versions.
cc := env_var_or_default("CC", "clang")
# Cross-assembler for labs (host CC cannot emit rv32i): override with
# `riscv_cc=riscv64-unknown-elf-gcc just run 01-pixel` if you have a
# bare-metal RISC-V GCC instead of clang.
riscv_cc := env_var_or_default("RISCV_CC", "clang")
cflags := "-std=c23 -O2 -Wall -Wextra -Wpedantic -Wshadow -Wundef -Werror=implicit-function-declaration -Werror=return-type -fdiagnostics-color=always -fcolor-diagnostics -I."
ldflags := "-lncursesw"

# Build VM (replaces Makefile)
build:
    {{cc}} {{cflags}} -c vm.c -o vm.o
    {{cc}} {{cflags}} -c ui.c -o ui.o
    {{cc}} {{cflags}} -c main.c -o main.o
    {{cc}} -o {{vm}} vm.o ui.o main.o {{ldflags}}

clean:
    rm -f vm.o ui.o main.o {{vm}} {{bindir}}/*.elf {{bindir}}/*.bin

# Assemble lab: prog.s -> elf -> bin
assemble lab="01-pixel":
    #!/bin/bash
    set -e
    src="labs/{{lab}}/prog.s"
    elf="{{bindir}}/{{lab}}.elf"
    bin="{{bindir}}/{{lab}}.bin"
    echo "==> {{lab}}: $src"
    {{riscv_cc}} --target=riscv32 -march=rv32i -nostdlib -Wl,-Ttext=0x600,--image-base=0x600 -o $elf $src
    llvm-objcopy -O binary --only-section=.text $elf $bin
    echo "ELF:"; llvm-objdump -d $elf | head -n 40
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

# Inspection
disasm lab="01-pixel":
    llvm-objdump -d {{bindir}}/{{lab}}.elf

elf lab="01-pixel":
    llvm-readelf -h -S -l {{bindir}}/{{lab}}.elf

hex lab="01-pixel":
    xxd {{bindir}}/{{lab}}.bin
