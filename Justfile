set shell := ["bash","-cu"]

vm := "vm"
bindir := "/data/data/com.termux/files/usr/tmp"

# Build VM
build:
    make -C . -j4

# Lab helper: monta prog.s -> bin em /tmp e roda
assemble lab="01-pixel":
    #!/bin/bash
    set -e
    src="labs/{{lab}}/prog.s"
    elf="{{bindir}}/{{lab}}.elf"
    bin="{{bindir}}/{{lab}}.bin"
    echo "==> {{lab}}: $src"
    clang --target=riscv32 -march=rv32i -nostdlib -Wl,-Ttext=0x0,--image-base=0x0 -o $elf $src
    llvm-objcopy -O binary $elf $bin
    echo "ELF:"; llvm-objdump -d $elf | head -n 40
    echo "BIN:"; xxd $bin | head -n 5
    echo "size $(wc -c < $bin) bytes"

run lab="01-pixel": (assemble lab)
    ./vm {{bindir}}/{{lab}}.bin --headless

debug lab="01-pixel": (assemble lab)
    ./vm {{bindir}}/{{lab}}.bin

# todos labs
all:
    just assemble 01-pixel
    just run 01-pixel
    just assemble 02-fib
    just run 02-fib
    just assemble 03-fib-ram
    just run 03-fib-ram

# inspeção igual linux_user_mode
disasm lab="01-pixel":
    llvm-objdump -d {{bindir}}/{{lab}}.elf
elf lab="01-pixel":
    llvm-readelf -h -S -l {{bindir}}/{{lab}}.elf
hex lab="01-pixel":
    xxd {{bindir}}/{{lab}}.bin
