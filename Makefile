CC := /opt/homebrew/opt/llvm/bin/clang
LD := wasm-ld
CFLAGS  := -Wall -Wextra -O3 -pedantic -std=c23 --target=wasm32 -flto -nostdlib
LDFLAGS := --no-entry --export-all --lto-O3 --export-memory

chip8.wasm: chip8.o printf.o
	$(LD) $(LDFLAGS) -o chip8.wasm chip8.o printf.o

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm chip8.wasm chip8.o printf.o
