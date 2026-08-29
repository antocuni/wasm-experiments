# Design: `ziglang-wasip2-sysroot` for SPy

Goal: let SPy compile emitted C to `wasm32-wasip2` (real sockets, files, clocks,
etc.) while keeping the "compiler is installed automatically via pip" experience
that `ziglang` gives today. Avoid bundling the full ~200 MB wasi-sdk.

## How the pieces relate

`wasi-sdk` is three separable things:

1. **A compiler** = clang/LLVM + `wasm-ld`. SPy already gets this from `ziglang`
   (`zig cc`, and `zig wasm-ld` internally uses LLD).
2. **A sysroot** = wasi-libc *headers* + *pre-compiled* archives (`libc.a`,
   `crt1*.o`, `libclang_rt.builtins.a`, the wasi-emulated-* shims) for a given
   target such as `wasm32-wasip2`. This is plain data, no executable needed.
3. **`wasm-component-ld`** = a small standalone binary that runs `wasm-ld` and
   then wraps the resulting core module into a WASI Preview 2 *component*,
   embedding a version-matched p1->p2 adapter.

Only (2) and (3) are missing from a `ziglang`-only install. Bundling just those
is small.

## Why not "just zig cc"?

- Real `socket()`/`connect()`/`bind()`/`listen()` exist only on the wasip2
  target of wasi-libc. On wasip1 you only get `accept`/`recv`/`send` on a
  host-provided fd, and modern wasmtime removed the preopened-listener path.
- `zig cc` **force-injects its own bundled wasi-libc headers** for any `wasm*-wasi*`
  triple, ahead of `-nostdinc`/`-isystem`. Those headers lag upstream and (in
  0.13/0.15) don't even declare sockets. zig 0.16 ships newer headers that *do*
  declare them under `-D__wasilibc_use_wasip2`, but zig still bundles **no
  wasip2 socket implementation**, so linking fails with `undefined symbol: socket`.

## The working recipe (validated end-to-end)

Compile with zig but feed it the **external** sysroot's headers, and link the
object against the **external** sysroot's libs:

```sh
# 1. compile: target a NON-wasi triple so zig does NOT inject its own headers,
#    then re-assert __wasi__ and point at the bundled wasip2 headers.
#    -Dmain=__main_argc_argv restores the wasi "main rename" that the wasi
#    target normally performs (crt1 calls __main_void -> __main_argc_argv).
zig cc --target=wasm32-freestanding -D__wasi__ -Dmain=__main_argc_argv \
       -nostdinc -isystem $SYSROOT/include/wasm32-wasip2 \
       -O2 -c -o prog.o prog.c

# 2. link + componentize: wasm-component-ld drives an external wasm-ld (zig's)
#    and produces a Preview 2 component using the bundled wasip2 libc.
wasm-component-ld --wasm-ld-path ./zig-wasm-ld \
    -L$SYSROOT/lib/wasm32-wasip2 \
    $SYSROOT/lib/wasm32-wasip2/crt1-command.o \
    prog.o -lc $SYSROOT/lib/wasm32-wasip2/libclang_rt.builtins.a \
    -o prog.wasm

# 3. run
wasmtime run -S inherit-network -S tcp prog.wasm
```

where `zig-wasm-ld` is a one-line shim:

```sh
#!/bin/sh
exec /path/to/ziglang/zig wasm-ld "$@"
```

Notes / gotchas discovered:

- `--target=wasm32-freestanding` is the key trick to stop zig hijacking headers.
  You must re-add `-D__wasi__` (wasi-libc headers guard on it) and
  `-Dmain=__main_argc_argv` (otherwise `main` stays plain `main`, crt1's
  `__main_void` can't find `__main_argc_argv`, and the module traps at startup
  with `undefined_weak:main`).
- The componentizer must be **version-matched** to the sysroot's embedded
  component-type metadata. `wasm-tools component new` from a mismatched release
  panics (`wit-component ... assertion failed: prev.is_none()`). `wasm-component-ld`
  ships its own matching adapter, so bundling it (rather than relying on an
  external `wasm-tools`) is the robust choice.

## Proposed package: `ziglang-wasip2-sysroot`

Contents (C-only; drop the bundled libc++ headers):

```
ziglang_wasip2_sysroot/
  include/wasm32-wasip2/**.h        # ~1.2 MB  (arch-independent)
  lib/wasm32-wasip2/
    libc.a  librt.a  crt1-command.o  crt1-reactor.o
    libclang_rt.builtins.a
    libwasi-emulated-*.a            # ~4.2 MB  (arch-independent wasm)
  bin/<host>/wasm-component-ld      # ~8.4 MB  (per host platform)
```

Sizes measured from wasi-sdk-34: **~14 MB unpacked, ~4 MB gzipped** for the
data, plus one ~8 MB host binary. Compare to ~200 MB for the full wasi-sdk.

Distribution options, in order of preference:

1. **Ship prebuilt archives + headers** (what wasi-sdk already publishes as
   `wasi-sysroot-XX.tar.gz`). The wasm libs and headers are host-independent, so
   one wheel serves all platforms; only `wasm-component-ld` is per-host (publish
   platform wheels like `ziglang` does, or vendor the 4 common hosts).
2. **Build wasi-libc on the fly** the way zig builds musl. Possible but heavy:
   it needs the wasi-libc sources plus a `wit-bindgen`-generated wasi-sockets
   binding step. Not worth it for wasm, which is already portable -- prefer (1).

`create-bundle.sh` in this directory produces the option-1 layout from an
installed wasi-sdk, as a concrete reference.

## Version pinning

Pin three things together, because the component-type ABI is versioned:
`wasi-sdk` (for the sysroot + `wasm-component-ld`), the `wasmtime` you run on,
and (optionally) `zig`. wasi-sdk-34 + wasmtime 48 were validated here.

## Open questions for SPy

- SPy emits C-only, so the libc++ headers/archives can be dropped (done above).
- If SPy needs threads, add the `wasm32-wasip2` equivalent thread libs (or use
  the `-threads` sysroot variant); not needed for the sockets demo.
- `crt1-command.o` gives a normal `main()` command component. Use
  `crt1-reactor.o` + `--wasi-adapter reactor` if SPy ever needs a library-style
  reactor component instead.
