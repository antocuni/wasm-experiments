# inspired by:
# https://depth-first.com/articles/2019/10/16/compiling-c-to-webassembly-and-running-it-without-emscripten/

# see also:
# https://github.com/ern0/howto-wasm-minimal/blob/master/build.sh
# https://github.com/ern0/howto-wasm-minimal
# https://news.ycombinator.com/item?id=30991235

# NOTE: you need to have recent clang installed. In my (antocuni) machine, run:
#     . ~/bin/activate-clang-15

build/mylib.wasm: build mylib.c
	@clang \
		--target=wasm32 \
		-nostdlib \
		-Wl,--export-all -Wl,--no-entry, -Wl,--allow-undefined \
		-g \
		-O3 \
		-o build/mylib.wasm \
		mylib.c

build:
	mkdir build

.PHONY: wasm2wat
wasm2wat: build/mylib.wasm
	wasm2wat --generate-names build/mylib.wasm | pygmentize -l wast -P style=rrt
