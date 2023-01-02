# NOTE: the following links do not mention --no-entry, but I had to add it to
# emcc options to allow the code to compile
#
# for compiling C and manually run WASM from JS:
#   https://v8.dev/blog/emscripten-standalone-wasm
#   https://github.com/emscripten-core/emscripten/wiki/WebAssembly-Standalone
#   https://dassur.ma/things/c-to-webassembly/

# for wasm2wat, wasm-decompile, etc.:
#   https://github.com/WebAssembly/wabt
#   https://github.com/WebAssembly/wabt/releases/tag/1.0.29

# for python
#   pip install wasmtime
#   https://github.com/bytecodealliance/wasmtime-py
#   https://bytecodealliance.github.io/wasmtime-py/


# useful CFLAGS to experiment with:
#   -O3: makes it easier to read the disassembled output in e.g. wasm2wat
#
#   -Wl,--export-all: exports all symbols, needed to access e.g. __heap_base,
#                     __data_end, etc. as described in the c-to-webassembly
#                     blogpost above
#CFLAGS=-O3 -Wl,--export-all

.PHONY: run
run: build/mylib.wasm
	@node run.js

.PHONY: run-python
run-python: build/mylib.wasm
	@python3 run.py

.PHONY: wasm2wat
wasm2wat: build/mylib.wasm
	wasm2wat --generate-names build/mylib.wasm | pygmentize -l wast -P style=rrt

.PHONY: wasm-decompile
wasm-decompile: build/mylib.wasm
	wasm-decompile build/mylib.wasm

.PHONY: wasm-objdump
wasm-objdump: build/mylib.wasm
	wasm-objdump -h build/mylib.wasm
	@echo
	@echo "try also -d, -s, -x and --help"


build:
	mkdir build

build/mylib.wasm: build mylib.c
	emcc mylib.c -o build/mylib.wasm --no-entry $(CFLAGS)
