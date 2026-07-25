/*
 * Demonstrate that bdwgc on wasi cannot see GC-managed pointers that
 * clang keeps in wasm locals (never spilled to the shadow stack).
 *
 * `p` is passed as a function parameter to check_after_gc(), so it
 * lives in a wasm local. Wasm locals persist across calls for free
 * (no caller-saves ABI), so clang has no reason to spill p onto the
 * shadow stack around the GC_gcollect() call. bdwgc scans the data
 * segment and the shadow stack — neither contains p — so the block
 * is treated as unreachable, reclaimed, and reused.
 *
 * All reads through p happen inside check_after_gc(), an opaque
 * (noinline) function, so clang can't constant-fold the reads back
 * into the source-level bytes we wrote with strcpy.
 */
#include <stdio.h>
#include <string.h>
#include <gc.h>

#define CHUNK 1024
static const char MAGIC[] = "hello-from-the-past";

static void __attribute__((noinline)) burn_through_heap(void) {
    /* Churn the freelist so the freed block gets reused. */
    for (int i = 0; i < 20000; i++) {
        char *q = GC_MALLOC(CHUNK);
        memset(q, 0xAA, CHUNK);
    }
}

static void __attribute__((noinline)) check_after_gc(char *p) {
    GC_gcollect();
    burn_through_heap();

    printf("first bytes at p after GC + reuse: ");
    for (int i = 0; i < (int)(sizeof(MAGIC) - 1); i++)
        printf("%02x ", (unsigned char)p[i]);
    printf("\n");
    printf("expected (MAGIC):                  ");
    for (int i = 0; i < (int)(sizeof(MAGIC) - 1); i++)
        printf("%02x ", (unsigned char)MAGIC[i]);
    printf("\n");

    if (memcmp(p, MAGIC, sizeof(MAGIC) - 1) == 0) {
        printf("=> OK");
    } else {
        printf("=> BUG! object was reclaimed and reused!\n");
    }
}

int main(void) {
    GC_INIT();

    char *p = GC_MALLOC(CHUNK);

    // spill the pointer: by uncommenting this, we force clang to place 'p' onto the
    // shadowstack, so bdwgc can find it and it works.
    // printf("&p = %p\n", &p);

    strcpy(p, MAGIC);

    /*
     * Deliberately do NOT read p here between the strcpy and the call —
     * that would give clang reasons to spill p to the shadow stack,
     * masking the bug.
     */
    check_after_gc(p);
    return 0;
}
