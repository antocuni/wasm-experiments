CFLAGS=-mbulk-memory -O3


run-wasmtime: hello.wasm
	wasmtime hello.wasm

run-node: hello.wasm run.mjs node_modules
	node run.mjs

run-python: hello.wasm
	python run.py

hello.o: hello.c
	zig cc --target=wasm32-wasi-musl $(CFLAGS) -c hello.c

hello.wasm: hello.o
	zig cc --target=wasm32-wasi-musl hello.o -o hello.wasm

node_modules:
	npm i
