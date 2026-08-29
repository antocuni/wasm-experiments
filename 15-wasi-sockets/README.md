# 15 - WASI sockets

Goal: write a small C program that uses TCP sockets, compile it to WASI with
`zig cc`, and run it with wasmtime -- and figure out a lightweight,
pip-installable toolchain for SPy (which emits C and compiles it to wasip2).

## TL;DR

- Real BSD sockets (`socket`/`bind`/`listen`/`connect`/`accept`) live on the
  **`wasm32-wasip2`** target and run on **wasmtime 48** with
  `-S inherit-network -S tcp`. They build into a WASI Preview 2 *component*.
- You **can** use `zig cc` as the compiler (and `zig wasm-ld` as the linker),
  but you must feed it an **external wasip2 sysroot** (headers + `libc.a` +
  crt + builtins) and a `wasm-component-ld` helper -- zig bundles none of the
  wasip2 socket implementation, and it force-injects its own outdated headers
  unless you use the `--target=wasm32-freestanding` trick below.
- That external sysroot is small (**~14 MB unpacked, ~4 MB gzipped**, C-only),
  which makes a `ziglang-wasip2-sysroot` companion package practical. See
  `DESIGN.md`.

## Quick start

```sh
python3 -m venv venv-zig016 && venv-zig016/bin/pip install ziglang==0.16
./make-zig.sh   # compile with zig cc, link+componentize -> server/client.wasm
./run.sh        # start the server, run the client, print the echo round-trip
```

Expected output:

```
connected to 127.0.0.1:8080
sent: hello from a WASI socket
echo reply: hello from a WASI socket
```

## Files

- `server.c` - TCP echo server: `socket`/`bind`/`listen`/`accept`/`recv`/`send`.
- `client.c` - TCP client: `socket`/`connect`/`send`/`recv`.
- `make-zig.sh` - build with `zig cc` + external wasip2 sysroot (the SPy path).
- `make-wasi-sdk.sh` - reference build with plain wasi-sdk `wasm32-wasip2-clang`.
- `run.sh` - start the server, run the client, print the round-trip.
- `clean.sh` - remove build artifacts.
- `config.sh` - shared paths, sourced by the other scripts.
- `DESIGN.md` - how to package a small `ziglang-wasip2-sysroot` for SPy.
- `create-bundle.sh` - assembles that minimal C-only bundle from a wasi-sdk.
- `zig-wasip1-accept.c` - historical: the only socket-ish thing plain `zig cc`
  (no external sysroot) can compile, the wasip1 accept-subset.

## The build recipe

```sh
# compile: freestanding target stops zig hijacking headers; re-assert __wasi__
# and rename main so wasi crt1's __main_void -> __main_argc_argv resolves.
zig cc --target=wasm32-freestanding -D__wasi__ -Dmain=__main_argc_argv \
       -nostdinc -isystem $SYSROOT/include/wasm32-wasip2 -O2 -c -o prog.o prog.c

# link + componentize, driving zig's own wasm-ld
wasm-component-ld --wasm-ld-path ./zig-wasm-ld \
    -L$SYSROOT/lib/wasm32-wasip2 $SYSROOT/lib/wasm32-wasip2/crt1-command.o \
    prog.o -lc $SYSROOT/lib/wasm32-wasip2/libclang_rt.builtins.a -o prog.wasm

# run
wasmtime run -S inherit-network -S tcp prog.wasm 127.0.0.1 8080
```

## Why plain `zig cc` alone can't do it

1. **wasi-libc split.** `socket()`/`connect()`/`bind()`/`listen()` exist only on
   the wasip2 target. On wasip1 you only get `accept`/`recv`/`send` on a
   host-provided fd, and modern wasmtime removed the old preopened-listener
   path (`-S tcplisten` + `-S preview2=n` -- `preview2=n` is a hard error since
   wasmtime 47; `tcplisten` now reports "components do not support --tcplisten").
2. **zig hijacks headers.** For any `wasm*-wasi*` triple, `zig cc` force-injects
   its own bundled `wasm-wasi-musl` headers ahead of `-nostdinc`/`-isystem`.
   zig 0.13/0.15 headers don't declare sockets at all; zig 0.16 headers declare
   them under `-D__wasilibc_use_wasip2` but zig ships **no wasip2 socket libc**,
   so it links with `undefined symbol: socket`. The fix is to compile as
   `--target=wasm32-freestanding` (zig won't inject) and point at an external
   wasip2 sysroot, as shown above.

## Toolchain installed for this experiment (needed network)

- **wasmtime 48.0.1** at `/home/antocuni/wasm/wasmtime-48`, symlinked from
  `/home/antocuni/wasm/bin/wasmtime` (replaced the old 11.0.1; revert with
  `ln -sf ../wasmtime/bin/wasmtime /home/antocuni/wasm/bin/wasmtime`).
- **wasi-sdk-34** at `/home/antocuni/wasm/wasi-sdk-34` -- used here as the source
  of the wasip2 sysroot + `wasm-component-ld`. A real deployment would ship the
  ~14 MB C-only subset produced by `create-bundle.sh`, not the whole SDK.
- **ziglang 0.16** in the local `venv-zig016/` (gitignored).
