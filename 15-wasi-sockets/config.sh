#!/bin/bash
# Shared configuration for the wasi-sockets build/run scripts.
# Sourced by make-zig.sh, make-wasi-sdk.sh and run.sh.

# --- wasi-sdk: source of the wasip2 sysroot + wasm-component-ld -------------
# For this experiment we point at a full wasi-sdk-34 install directly. A real
# deployment would ship only the ~14 MB C-only subset produced by create-bundle.sh.
SDK=/home/antocuni/wasm/wasi-sdk-34
SYSROOT=$SDK/share/wasi-sysroot
SYSINC=$SYSROOT/include/wasm32-wasip2
SYSLIB=$SYSROOT/lib/wasm32-wasip2
BUILTINS=$SDK/lib/clang/23/lib/wasm32-unknown-wasip2/libclang_rt.builtins.a
COMPONENTLD=$SDK/bin/wasm-component-ld

# --- zig 0.16 from the local venv ------------------------------------------
# (python3 -m venv venv-zig016 && venv-zig016/bin/pip install ziglang==0.16)
ZIG_BIN=$(./venv-zig016/bin/python -c "import ziglang,os;print(os.path.join(os.path.dirname(ziglang.__file__),'zig'))")

# --- wasmtime --------------------------------------------------------------
WASMTIME=/home/antocuni/wasm/bin/wasmtime

CFLAGS="-O2 -Wall"
