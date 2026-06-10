wasm-tools component embed demo.wit demo.wat -o demo.wasm
wasm-tools component new demo.wasm -o demo.component.wasm
python -m wasmtime.bindgen demo.component.wasm --out-dir demo
