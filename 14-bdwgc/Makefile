CC       := zig cc
TARGET   := --target=wasm32-wasi-musl
CFLAGS   := $(TARGET) -O2 -mbulk-memory
WASMTIME := wasmtime

BDWGC_DIR := bdwgc
BDWGC_INC := -I$(BDWGC_DIR)/include
BDWGC_DEFS := -DGC_BUILTIN_ATOMIC -DNO_EXECUTE_PERMISSION

.PHONY: all run run-gc clean

all: leak.wasm leak_gc.wasm

leak.wasm: leak.c
	$(CC) $(CFLAGS) $< -o $@

libgc.o: $(BDWGC_DIR)/extra/gc.c
	$(CC) $(CFLAGS) $(BDWGC_INC) $(BDWGC_DEFS) -c $< -o $@

leak_gc.wasm: leak_gc.c libgc.o
	$(CC) $(CFLAGS) $(BDWGC_INC) leak_gc.c libgc.o -o $@

run: leak.wasm
	$(WASMTIME) leak.wasm

run-gc: leak_gc.wasm
	$(WASMTIME) leak_gc.wasm

clean:
	rm -f leak.wasm leak_gc.wasm libgc.o
