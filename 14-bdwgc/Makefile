CC       := zig cc
TARGET   := --target=wasm32-wasi-musl
CFLAGS   := $(TARGET) -O2 -mbulk-memory
WASMTIME := wasmtime

.PHONY: all run clean

all: leak.wasm

leak.wasm: leak.c
	$(CC) $(CFLAGS) $< -o $@

run: leak.wasm
	$(WASMTIME) leak.wasm

clean:
	rm -f leak.wasm
