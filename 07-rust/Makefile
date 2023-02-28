# https://surma.dev/things/rust-to-webassembly/
#
# NOTE: you need to install the wasm rust target with:
#   rustup target add wasm32-unknown-unknown

mylib_wasm = target/wasm32-unknown-unknown/release/mylib.wasm

# .phony here is a hack to convince make to rebuild mylib.wasm every time
.PHONY: $(mylib_wasm)
$(mylib_wasm):
	cargo build --target=wasm32-unknown-unknown --release


.PHONY: wasm2wat
wasm2wat: $(mylib_wasm)
	wasm2wat --generate-names -f $(mylib_wasm) | pygmentize -l wast -P style=rrt

.PHONY: llvm-objdump
llvm-objdump: $(mylib_wasm)
	llvm-objdump -h $(mylib_wasm)

.PHONY: llvm-objdump-antocuni
llvm-objdump-antocuni: $(mylib_wasm)
	llvm-objdump -s -j antocuni-custom-section $(mylib_wasm)


.PHONY: wasm-objdump
wasm-objdump: $(mylib_wasm)
	wasm-objdump -h $(mylib_wasm)

.PHONY: twiggy-top
twiggy-top: $(mylib_wasm)
	# try to disable stripping in Cargo.toml and see
	twiggy top $(mylib_wasm)
