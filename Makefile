CC=clang
CFLAGS=-O2 -Wall -Wextra -I.
LDFLAGS=-lncursesw

SRC=vm.c ui.c main.c
OBJ=$(SRC:.c=.o)
BIN=vm

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c vm.h ui.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

run: $(BIN)
	./$(BIN) ~/Languages/asm/riscv/fibonacci.bin

ram: $(BIN)
	./$(BIN) ~/Languages/asm/riscv/fib_ram.bin

headless:
	./$(BIN) ~/Languages/asm/riscv/fibonacci.bin --headless
	./$(BIN) ~/Languages/asm/riscv/fib_ram.bin --headless

kernel: $(BIN)
	./$(BIN) ~/Languages/asm/riscv/kernel.bin --headless

.PHONY: all clean run ram headless kernel
