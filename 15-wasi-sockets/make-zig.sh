#!/bin/bash
# Build server.wasm + client.wasm the "SPy-friendly" way (the recommended route
# from README.md): ziglang + wasi-libc + wasm-tools, no wasm-component-ld.
#   1. compile ORDINARY socket C with `zig cc`  (uses the external wasip2 headers)
#   2. link to a core module with zig's own `zig wasm-ld` (external wasip2 libc)
#   3. componentize with `wasm-tools component new`
#
# zig does not bundle the wasip2 socket libc, and it force-injects its own
# outdated wasi-libc headers for any wasm*-wasi* triple. We dodge that by
# compiling as --target=wasm32-freestanding (so zig does NOT inject headers),
# re-asserting __wasi__, and pointing -isystem at the external sysroot.
# -Dmain=__main_argc_argv restores the wasi "main rename" that the wasi target
# normally performs (crt1 calls __main_void -> __main_argc_argv); without it the
# module traps at startup with `undefined_weak:main`.
set -euo pipefail
cd "$(dirname "$0")"
source ./config.sh

ZIG_CC=("$ZIG_BIN" cc --target=wasm32-freestanding -D__wasi__
        -Dmain=__main_argc_argv -nostdinc -isystem "$SYSINC" $CFLAGS)

build() {
    local name=$1
    echo ">>> [1/3] compiling $name.c with zig cc"
    "${ZIG_CC[@]}" -c -o "$name.o" "$name.c"
    echo ">>> [2/3] linking $name.core.wasm with zig wasm-ld"
    "$ZIG_BIN" wasm-ld -L"$SYSLIB" "$SYSLIB/crt1-command.o" "$name.o" \
        -lc "$BUILTINS" -o "$name.core.wasm"
    echo ">>> [3/3] componentizing $name.wasm with wasm-tools"
    "$WASM_TOOLS" component new "$name.core.wasm" -o "$name.wasm"
}

build server
build client
echo ">>> done: server.wasm client.wasm"
