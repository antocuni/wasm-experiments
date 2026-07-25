#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WASM_PAGE_SIZE 65536

static size_t wasm_mem_bytes(void) {
    size_t pages = (size_t)__builtin_wasm_memory_size(0);
    __asm__ __volatile__("" : "+r"(pages));
    return pages * WASM_PAGE_SIZE;
}

int main(void) {
    const int iterations = 2000;
    const size_t chunk = 256 * 1024;

    printf("initial memory: %zu bytes (%zu pages)\n",
           wasm_mem_bytes(), wasm_mem_bytes() / WASM_PAGE_SIZE);

    for (int i = 0; i < iterations; i++) {
        char *p = malloc(chunk);
        if (!p) {
            printf("malloc failed at iteration %d\n", i);
            break;
        }
        memset(p, i & 0xff, chunk);
        // intentionally leak: no free(p)

        if ((i + 1) % 100 == 0) {
            printf("after %4d allocs of %zu bytes: memory = %zu bytes, last ptr = %p\n",
                   i + 1, chunk, wasm_mem_bytes(), (void *)p);
        }
    }

    printf("final memory:   %zu bytes (%zu pages)\n",
           wasm_mem_bytes(), wasm_mem_bytes() / WASM_PAGE_SIZE);
    return 0;
}
