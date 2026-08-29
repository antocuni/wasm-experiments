#!/bin/bash
# Build server.wasm + client.wasm the "SPy-friendly" way:
#   - compile with `zig cc`
#   - link + componentize with wasm-component-ld, driving zig's own `wasm-ld`
#   - using an external wasip2 sysroot (headers + libc.a + crt + builtins)
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

# Shim so wasm-component-ld (which shells out to an external wasm-ld) uses zig's.
printf '#!/bin/sh\nexec %s wasm-ld "$@"\n' "$ZIG_BIN" > zig-wasm-ld
chmod +x zig-wasm-ld

build() {
    local name=$1
    echo ">>> compiling $name.c with zig cc"
    "${ZIG_CC[@]}" -c -o "$name.o" "$name.c"
    echo ">>> linking + componentizing $name.wasm (zig wasm-ld)"
    "$COMPONENTLD" --wasm-ld-path ./zig-wasm-ld \
        -L"$SYSLIB" "$SYSLIB/crt1-command.o" "$name.o" -lc "$BUILTINS" \
        -o "$name.wasm"
}

build server
build client
echo ">>> done: server.wasm client.wasm"
