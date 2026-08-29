#!/bin/bash
# Reference build using the full wasi-sdk toolchain directly: wasi-sdk's own
# clang (wasm32-wasip2) does both the compile and the link+componentize. This
# does NOT use zig at all -- it's the baseline that make-zig.sh reproduces with
# zig as the compiler/linker.
set -euo pipefail
cd "$(dirname "$0")"
source ./config.sh

for name in server client; do
    echo ">>> building $name.wasm with wasi-sdk wasm32-wasip2-clang"
    "$SDK/bin/clang" --target=wasm32-wasip2 $CFLAGS -o "$name.wasm" "$name.c"
done
echo ">>> done: server.wasm client.wasm"
