CFLAGS=-mbulk-memory -O3


run-wasmtime: hello.wasm
	wasmtime hello.wasm

run-node: hello.wasm run.mjs node_modules
	node run.mjs

run-python: hello.wasm
	python run.py

hello.wasm: hello.c
	zig cc --target=wasm32-wasi-musl $(CFLAGS) hello.c -o hello.wasm

node_modules:
	npm i
