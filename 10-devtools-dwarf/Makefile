# NOTE you MIGHT need this chrom extension:
# https://chrome.google.com/webstore/detail/cc%20%20-devtools-support-dwa/pdcpmagijalfljmkmjngeonclgbbannb

# https://developer.chrome.com/blog/wasm-debugging-2020/
# https://developer.chrome.com/blog/faster-wasm-debugging/

# NOTE: as of2023-08-06 zig cc -gsplit-dwarf doesn't work properly:
# https://github.com/ziglang/zig/issues/11858
# https://github.com/ziglang/zig/issues/11194#issuecomment-1154784001

# The summary is:
#   1) clang -gsplit-dwarf creates *.dwo files (dwarf objects)
#   2) you can link *.o as usual
#   3) llvm-dwp -e searches all the *.dwo referenced by the given binary and
#      produces a final *.dwp (dwarf package)
#   4) for a given foo.wasm, chrome dev tools try to fetch foo.wasm.dwp

build/fibo.wasm: build/fibo.o
	cd build && clang \
        --target=wasm32 \
        -nostdlib \
        -Wl,--export=fibo \
        -Wl,--no-entry \
        -o fibo.wasm \
        fibo.o
	cd build && llvm-dwp-14 -e fibo.wasm -o fibo.wasm.dwp

build/fibo.o: fibo.c
	mkdir -p build
	cd build && clang \
        --target=wasm32 \
        -nostdlib \
        -Og \
        -g \
        -gsplit-dwarf \
        -gdwarf-5 \
        -c ../fibo.c


dump-dwarf: build/fibo.wasm
	@llvm-dwarfdump build/fibo.wasm
	@echo
	@llvm-dwarfdump build/fibo.wasm.dwp

serve:
	python -m http.server
