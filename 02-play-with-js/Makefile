# https://developer.mozilla.org/en-US/docs/WebAssembly/C_to_wasm
# https://emscripten.org/docs/porting/connecting_cpp_and_javascript/Interacting-with-code.html#interacting-with-code-ccall-cwrap

.PHONY: run
run: build/mylib.js
	node run.js

build:
	mkdir build

build/mylib.js: build mylib.c
	emcc mylib.c -o build/mylib.js \
        -sMODULARIZE \
        -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,allocateUTF8 \
        -sEXPORTED_FUNCTIONS=_malloc
