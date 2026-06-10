// Tiny guest module compiled to wasm32-wasi.
//
// We expose:
//   - linear memory (default export "memory")
//   - alloc(size) -> pointer (i32 offset into linear memory)
//   - sum(ptr, len) -> int : sums bytes the host has written
//   - first_byte(ptr) -> int : returns the first byte at ptr
//
// NOTE: the "pointer" returned by alloc() is a 32-bit offset into wasm
// linear memory, not a native host pointer. The host translates it by
// adding the base address of the wasm memory.

#include <stdlib.h>
#include <stdint.h>

__attribute__((export_name("alloc")))
void* guest_alloc(size_t n) {
    return malloc(n);
}

__attribute__((export_name("dealloc")))
void guest_dealloc(void* p) {
    free(p);
}

__attribute__((export_name("sum")))
int guest_sum(uint8_t* p, size_t n) {
    int s = 0;
    for (size_t i = 0; i < n; i++) s += p[i];
    return s;
}

__attribute__((export_name("first_byte")))
int guest_first_byte(uint8_t* p) {
    return (int)p[0];
}
