"""
Host-side writes into guest WebAssembly linear memory.

This script:
  1. Loads guest.wasm (compiled from guest.c).
  2. Calls the guest `alloc(N)` to obtain a buffer inside the wasm linear memory.
     The returned value is a 32-bit OFFSET into linear memory, not a native pointer.
  3. Obtains a host-side pointer to the base of the wasm linear memory and writes
     bytes directly into the guest buffer via ctypes.
  4. Calls guest `sum(ptr, len)` and `first_byte(ptr)` to prove the guest sees
     exactly the bytes the host wrote.

WARNING: do NOT keep raw host pointers/slices to wasm memory across wasm calls
or across memory growth. Any wasm call may trigger memory.grow, which can
relocate the linear memory in the host address space and invalidate previously
obtained pointers. Always reacquire `data_ptr` / `data_len` after each wasm
call (or at least after any call that might allocate/grow memory).

Install / run:
    pip install wasmtime
    # zig (with wasi-musl target) or recent clang must be on PATH
    make            # produces guest.wasm
    python run.py
"""

import ctypes
import wasmtime as wt


def main():
    engine = wt.Engine()
    module = wt.Module.from_file(engine, "guest.wasm")
    store = wt.Store(engine)

    # WASI is needed because guest.c links against wasi-musl (malloc pulls it in).
    wasi_config = wt.WasiConfig()
    wasi_config.inherit_stdout()
    wasi_config.inherit_stderr()
    store.set_wasi(wasi_config)

    linker = wt.Linker(engine)
    linker.define_wasi()
    instance = linker.instantiate(store, module)

    exports = instance.exports(store)
    memory: wt.Memory = exports["memory"]
    alloc = exports["alloc"]
    dealloc = exports["dealloc"]
    guest_sum = exports["sum"]
    first_byte = exports["first_byte"]

    payload = b"Hello, wasm linear memory!"
    n = len(payload)

    # `guest_ptr` is an i32 OFFSET into the wasm linear memory, not a host pointer.
    guest_ptr = alloc(store, n)
    print(f"guest alloc({n}) -> offset 0x{guest_ptr:x} (in linear memory)")

    # Acquire a host pointer to the base of the wasm linear memory.
    # NOTE: this pointer is only valid until the next wasm call that may grow
    # memory. Reacquire after each call to be safe.
    data_ptr = memory.data_ptr(store)        # ctypes.POINTER(c_ubyte)
    data_len = memory.data_len(store)        # int, in bytes
    assert guest_ptr + n <= data_len, "buffer overruns linear memory"

    # Write directly into guest memory at offset `guest_ptr` using ctypes.
    # `data_ptr` points to byte 0 of the guest's linear memory; adding
    # `guest_ptr` walks to the offset the guest told us about.
    dst = ctypes.addressof(data_ptr.contents) + guest_ptr
    ctypes.memmove(dst, payload, n)

    # Equivalent slice-style write (also valid before any further wasm call):
    # ctypes.cast(data_ptr, ctypes.POINTER(ctypes.c_ubyte * data_len))[0][guest_ptr:guest_ptr+n] = payload

    # Now ask the guest to read back what the host wrote.
    # IMPORTANT: after this call, treat `data_ptr` / `data_len` as potentially
    # stale and reacquire them if you need to touch memory again.
    s = guest_sum(store, guest_ptr, n)
    expected = sum(payload)
    fb = first_byte(store, guest_ptr)

    print(f"guest sum  = {s} (expected {expected})")
    print(f"guest first_byte = {fb} ({chr(fb)!r}, expected {payload[0]!r})")
    assert s == expected
    assert fb == payload[0]

    dealloc(store, guest_ptr)
    print("OK: host-written bytes visible to the guest.")


if __name__ == "__main__":
    main()
