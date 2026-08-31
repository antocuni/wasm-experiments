#!/bin/bash
# Assemble a minimal, C-only `ziglang-wasip2-sysroot` from an installed
# wasi-sdk. Produces the (data-only, host-independent) layout described in
# README.md:
#
#   <out>/include/wasm32-wasip2/**.h
#   <out>/lib/wasm32-wasip2/{libc.a,librt.a,crt1-*.o,libclang_rt.builtins.a,...}
#
# The componentizer is NOT bundled: the recommended route uses `wasm-tools`
# (published on PyPI). Pass INCLUDE_COMPONENTLD=1 to also copy the wasi-sdk
# `wasm-component-ld` (the discarded alternative -- see README) into <out>/bin.
#
# Usage: [INCLUDE_COMPONENTLD=1] ./create-bundle.sh [WASI_SDK_DIR] [OUT_DIR]
set -euo pipefail

SDK="${1:-/home/antocuni/wasm/wasi-sdk-34}"
OUT="${2:-./ziglang-wasip2-sysroot}"

SR="$SDK/share/wasi-sysroot"
CLANG_LIB="$(dirname "$(find "$SDK/lib/clang" -name 'libclang_rt.builtins.a' -path '*wasip2*' | head -1)")"

rm -rf "$OUT"
mkdir -p "$OUT/include" "$OUT/lib/wasm32-wasip2"

# Headers (C only: drop the bundled libc++ trees, which dominate the size).
cp -r "$SR/include/wasm32-wasip2" "$OUT/include/"
rm -rf "$OUT/include/wasm32-wasip2/c++" \
       "$OUT/include/wasm32-wasip2/eh" \
       "$OUT/include/wasm32-wasip2/noeh"

# Core archives + startup objects + emulated shims (all host-independent wasm).
cp "$SR/lib/wasm32-wasip2/libc.a" \
   "$SR/lib/wasm32-wasip2/librt.a" \
   "$SR/lib/wasm32-wasip2/crt1-command.o" \
   "$SR/lib/wasm32-wasip2/crt1-reactor.o" \
   "$OUT/lib/wasm32-wasip2/"
cp "$SR"/lib/wasm32-wasip2/libwasi-emulated-*.a "$OUT/lib/wasm32-wasip2/" 2>/dev/null || true
cp "$CLANG_LIB/libclang_rt.builtins.a" "$OUT/lib/wasm32-wasip2/"

# Optional: the wasi-sdk componentizer (the discarded alternative). Only copied
# when explicitly requested; the recommended route uses `wasm-tools` from PyPI.
if [ "${INCLUDE_COMPONENTLD:-0}" = "1" ]; then
    mkdir -p "$OUT/bin"
    cp "$SDK/bin/wasm-component-ld" "$OUT/bin/"   # ships its own version-matched p1->p2 adapter
fi

echo "bundle written to: $OUT"
du -sh "$OUT" "$OUT/include" "$OUT/lib"
[ -d "$OUT/bin" ] && du -sh "$OUT/bin"
