# https://developer.mozilla.org/en-US/docs/WebAssembly/C_to_wasm

EMSDK=/home/antocuni/anaconda/emsdk

.PHONY: run-browser
run-browser: build/hello.html
	emrun build/hello.html

.PHONY: run-node
run-node: build/hello.js
	node build/hello.js

build:
	mkdir build

build/hello.html: build hello.c
	emcc hello.c -o build/hello.html --shell-file $(EMSDK)/upstream/emscripten//src/shell_minimal.html

build/hello.js: build hello.c
	emcc hello.c -o build/hello.js
