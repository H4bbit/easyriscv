set shell := ["bash", "-cu"]

vm := "vm"
# Portable scratch dir for .elf/.bin: honors $TMPDIR (Termux, macOS,
# Nix) with /tmp fallback. Never hardcode an OS-specific tmp path here.
bindir := env_var_or_default("TMPDIR", "/tmp")
# Host compiler: default clang (only toolchain tested here so far).
# Ready for future gcc testing: `CC=gcc just build` (no clang-only flags
# in cflags on purpose; please report breakage).
cc := env_var_or_default("CC", "clang")
# Portable C23 flags, overridable as a whole: `CFLAGS="..." just build`.
# Keep this default free of clang-only and gcc-only options.
cflags := env_var_or_default("CFLAGS", "-std=c23 -O2 -Wall -Wextra -Wpedantic -Wshadow -Wundef -Werror=implicit-function-declaration -Werror=return-type -fdiagnostics-color=always -I.")
# Reference RISC-V toolchain (ground truth for disasm/elf/check-asm):
# default clang+llvm, swappable for future gcc testing, e.g.
# RISCV_CC=riscv64-unknown-elf-gcc RISCV_FLAGS="-march=rv32i -mabi=ilp32 -nostdlib -Wl,-T,riscv.ld"
#   OBJCOPY=riscv64-unknown-elf-objcopy OBJDUMP=riscv64-unknown-elf-objdump READELF=riscv64-unknown-elf-readelf
# (untested path — clang output is the current source of truth).
riscv_cc := env_var_or_default("RISCV_CC", "clang")
riscv_flags := env_var_or_default("RISCV_FLAGS", "--target=riscv32 -march=rv32i -nostdlib -Wl,-T,riscv.ld")
objcopy := env_var_or_default("OBJCOPY", "llvm-objcopy")
objdump := env_var_or_default("OBJDUMP", "llvm-objdump")
readelf := env_var_or_default("READELF", "llvm-readelf")
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

# Assemble lab: prog.s -> bin via our own ./asm (byte-identical to the
# reference toolchain output — clang by default, see just check-asm).
# The reference toolchain stays as independent ground truth for
# inspection (disasm/elf/hex).
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

# Inspection (reference toolchain ground truth: clang+llvm by default,
# swappable via RISCV_CC/RISCV_FLAGS/OBJCOPY/OBJDUMP/READELF).
# Builds the reference ELF, proving decode_to_str against objdump.
disasm lab="01-pixel":
    {{riscv_cc}} {{riscv_flags}} -o {{bindir}}/{{lab}}.elf labs/{{lab}}/prog.s
    {{objdump}} -d {{bindir}}/{{lab}}.elf

elf lab="01-pixel":
    {{riscv_cc}} {{riscv_flags}} -o {{bindir}}/{{lab}}.elf labs/{{lab}}/prog.s
    {{readelf}} -h -S -l {{bindir}}/{{lab}}.elf

hex lab="01-pixel":
    xxd {{bindir}}/{{lab}}.bin

# Cross-check: our ./asm must be byte-identical to the reference toolchain
# for every lab and every solution. Catches drift between asm.c and the
# reference assembler.
check-asm:
    #!/bin/bash
    set -e
    [ -x ./asm ] || just build
    fail=0
    for src in labs/*/prog.s solutions/*/*.s; do \
      {{riscv_cc}} {{riscv_flags}} -o "{{bindir}}/ref.elf" "$src" 2>/dev/null; \
      {{objcopy}} -O binary --only-section=.text "{{bindir}}/ref.elf" "{{bindir}}/ref.bin"; \
      ./asm "$src" "{{bindir}}/my.bin"; \
      cmp -s "{{bindir}}/ref.bin" "{{bindir}}/my.bin" || { echo "DIFF $src"; fail=1; }; \
    done
    rm -f "{{bindir}}/ref.elf" "{{bindir}}/ref.bin" "{{bindir}}/my.bin"
    [ "$fail" = 0 ] && echo "check-asm: all labs+solutions byte-identical" || exit 1
