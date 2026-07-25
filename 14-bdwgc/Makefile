CC       := zig cc
TARGET   := --target=wasm32-wasi-musl
CFLAGS   := $(TARGET) -O2 -mbulk-memory
WASMTIME := wasmtime

EMCC     := /home/antocuni/anaconda/pyodide/emsdk/emsdk/upstream/emscripten/emcc
# NB: emsdk puts a directory literally named `node` on PATH via
# emsdk_env.sh (its Node distribution's parent dir). GNU make's fast
# exec path hits that dir first and fails with EACCES before finding
# the real binary, so pin the absolute path here.
NODE     := $(shell command -v node)
EMCFLAGS := -O2 -sALLOW_MEMORY_GROWTH=1

BDWGC_DIR := bdwgc
BDWGC_INC := -I$(BDWGC_DIR)/include
BDWGC_DEFS := -DGC_BUILTIN_ATOMIC -DNO_EXECUTE_PERMISSION

.PHONY: all clean \
        wasi-run-leak wasi-run-noleak wasi-run-lost wasi-run-spill \
        em-run-leak

all: leak.wasm gc_noleak.wasm lost_pointer.wasm spill_test.wasm leak.em.js

# --- wasi builds (run via wasmtime) ---

leak.wasm: leak.c
	$(CC) $(CFLAGS) $< -o $@

libgc.o: $(BDWGC_DIR)/extra/gc.c
	$(CC) $(CFLAGS) $(BDWGC_INC) $(BDWGC_DEFS) -c $< -o $@

gc_noleak.wasm: gc_noleak.c libgc.o
	$(CC) $(CFLAGS) $(BDWGC_INC) gc_noleak.c libgc.o -o $@

lost_pointer.wasm: lost_pointer.c libgc.o
	$(CC) $(CFLAGS) $(BDWGC_INC) lost_pointer.c libgc.o -o $@

spill_test.wasm: spill_test.c libgc.o
	$(CC) $(CFLAGS) $(BDWGC_INC) spill_test.c libgc.o -o $@

wasi-run-leak: leak.wasm
	$(WASMTIME) leak.wasm

wasi-run-noleak: gc_noleak.wasm
	$(WASMTIME) gc_noleak.wasm

wasi-run-lost: lost_pointer.wasm
	$(WASMTIME) lost_pointer.wasm

wasi-run-spill: spill_test.wasm
	$(WASMTIME) spill_test.wasm

# --- emscripten builds (run via node) ---

leak.em.js: leak.c
	$(EMCC) $(EMCFLAGS) $< -o $@

em-run-leak: leak.em.js
	$(NODE) leak.em.js

clean:
	rm -f leak.wasm gc_noleak.wasm lost_pointer.wasm spill_test.wasm libgc.o \
	      leak.em.js leak.em.wasm
