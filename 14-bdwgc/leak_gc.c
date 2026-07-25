#include <stdio.h>
#include <string.h>
#include <gc.h>

#define WASM_PAGE_SIZE 65536

static size_t wasm_mem_bytes(void) {
    size_t pages = (size_t)__builtin_wasm_memory_size(0);
    __asm__ __volatile__("" : "+r"(pages));
    return pages * WASM_PAGE_SIZE;
}

int main(void) {
    GC_INIT();

    const int iterations = 2000;
    const size_t chunk = 256 * 1024;

    printf("initial memory: %zu bytes (%zu pages)\n",
           wasm_mem_bytes(), wasm_mem_bytes() / WASM_PAGE_SIZE);

    for (int i = 0; i < iterations; i++) {
        char *p = GC_MALLOC(chunk);
        if (!p) {
            printf("GC_MALLOC failed at iteration %d\n", i);
            break;
        }
        memset(p, i & 0xff, chunk);
        // no free: bdwgc should collect unreachable blocks.

        if ((i + 1) % 100 == 0) {
            printf("after %4d allocs of %zu bytes: memory = %zu bytes, heap = %zu bytes, last ptr = %p\n",
                   i + 1, chunk, wasm_mem_bytes(),
                   (size_t)GC_get_heap_size(), (void *)p);
        }
    }

    printf("final memory:   %zu bytes (%zu pages)\n",
           wasm_mem_bytes(), wasm_mem_bytes() / WASM_PAGE_SIZE);
    printf("final GC heap:  %zu bytes, %zu total allocd\n",
           (size_t)GC_get_heap_size(),
           (size_t)GC_get_total_bytes());
    return 0;
}
