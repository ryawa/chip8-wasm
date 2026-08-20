CC := /opt/homebrew/opt/llvm/bin/clang
CFLAGS  := -Wall -Wextra -O3 -pedantic -std=c23 --target=wasm32 -flto -nostdlib
LDFLAGS := -Wl,--no-entry -Wl,--export-all -Wl,--lto-O3 -Wl,--export-memory

chip8.wasm: chip8.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o chip8.wasm chip8.c
