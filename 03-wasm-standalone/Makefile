# NOTE: the following links do not mention --no-entry, but I had to add it to
# emcc options to allow the code to compile
#
# https://v8.dev/blog/emscripten-standalone-wasm
# https://github.com/emscripten-core/emscripten/wiki/WebAssembly-Standalone

# wasm2wat and wasm-decompile are available from here:
# https://github.com/WebAssembly/wabt
# https://github.com/WebAssembly/wabt/releases/tag/1.0.29


.PHONY: run
run: build/mylib.wasm
	node run.js

.PHONY: wasm2wat
wasm2wat: build/mylib.wasm
	wasm2wat build/mylib.wasm

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
	emcc mylib.c -o build/mylib.wasm --no-entry
