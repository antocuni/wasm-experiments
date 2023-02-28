# grep for MODULARIZE in this page:
# https://github.com/emscripten-core/emscripten/blob/main/src/settings.js#L1158

.PHONY: run
run: build/hello.js
	#node run.js

build:
	mkdir build

build/hello.js: build hello.c
	emcc hello.c -o build/hello.js \
        -sMODULARIZE \
        -sEXPORT_NAME=createHelloModule
