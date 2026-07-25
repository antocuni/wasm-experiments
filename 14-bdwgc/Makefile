CC       := zig cc
TARGET   := --target=wasm32-wasi-musl
CFLAGS   := $(TARGET) -O2 -mbulk-memory
WASMTIME := wasmtime

BDWGC_DIR := bdwgc
BDWGC_INC := -I$(BDWGC_DIR)/include
BDWGC_DEFS := -DGC_BUILTIN_ATOMIC -DNO_EXECUTE_PERMISSION

.PHONY: all run run-gc run-lost run-spill clean

all: leak.wasm leak_gc.wasm lost_pointer.wasm spill_test.wasm

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

lost_pointer.wasm: lost_pointer.c libgc.o
	$(CC) $(CFLAGS) $(BDWGC_INC) lost_pointer.c libgc.o -o $@

run-lost: lost_pointer.wasm
	$(WASMTIME) lost_pointer.wasm

spill_test.wasm: spill_test.c libgc.o
	$(CC) $(CFLAGS) $(BDWGC_INC) spill_test.c libgc.o -o $@

run-spill: spill_test.wasm
	$(WASMTIME) spill_test.wasm

clean:
	rm -f leak.wasm leak_gc.wasm lost_pointer.wasm spill_test.wasm libgc.o
