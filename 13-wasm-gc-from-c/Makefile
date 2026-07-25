EMSDK_BIN  := /home/antocuni/anaconda/pyodide/emsdk/emsdk/upstream/bin
CLANG      := $(EMSDK_BIN)/clang
WASM_MERGE := $(EMSDK_BIN)/wasm-merge
WASM_OPT   := $(EMSDK_BIN)/wasm-opt
WASM_TOOLS := wasm-tools
NODE       := node

CFLAGS := --target=wasm32 -mreference-types -nostdlib -O2 \
          -Wl,--no-entry -Wl,--export-dynamic

FEATURES    := --enable-gc --enable-reference-types
MERGE_FLAGS := $(FEATURES)
# Guaranteed inlining: bump the "always inline" size ceiling well above any
# point_* body, and strip point.* exports so DCE can drop the originals.
OPT_FLAGS   := $(FEATURES) -O3 \
               --always-inline-max-function-size=1000 \
               --inlining-optimizing

.PHONY: all run clean

all: combined.wasm

point.wasm: point.wat
	$(WASM_TOOLS) parse $< -o $@

main.wasm: main.c
	$(CLANG) $(CFLAGS) $< -o $@

combined.wasm: point.wasm main.wasm
	$(WASM_MERGE) point.wasm point main.wasm main -o $@ $(MERGE_FLAGS)

run: combined.wasm
	$(NODE) run.js combined.wasm

clean:
	rm -f point.wasm main.wasm combined.wasm
