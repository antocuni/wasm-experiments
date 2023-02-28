#https://emscripten.org/docs/porting/files/packaging_files.html

.PHONY: run
run: build/hellofs.js
	#node run.js

build:
	mkdir build

build/hellofs.js: build hellofs.c
	emcc hellofs.c -o build/hellofs.js -sINVOKE_RUN=0
