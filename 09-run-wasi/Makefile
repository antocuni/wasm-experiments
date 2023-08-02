run-wasmtime: hello.wasm
	wasmtime hello.wasm

run-node: hello.wasm run.mjs node_modules
	node run.mjs

hello.wasm: hello.c
	zig cc --target=wasm32-wasi-musl hello.c -o hello.wasm

node_modules:
	npm i
