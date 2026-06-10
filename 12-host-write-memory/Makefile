CFLAGS = -O2 -mbulk-memory

guest.wasm: guest.c
	zig cc --target=wasm32-wasi-musl $(CFLAGS) \
	    -mexec-model=reactor \
	    guest.c -o guest.wasm

run: guest.wasm
	python run.py

clean:
	rm -f guest.wasm
