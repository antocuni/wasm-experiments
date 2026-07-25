/*
 * Demonstrate that bdwgc on wasi cannot see GC-managed pointers that
 * clang keeps in wasm locals (never spilled to the shadow stack), and
 * show that GC_KEEP fixes the problem for a specific variable.
 *
 * We allocate two buffers:
 *   - kept: spilled to the shadow stack via GC_KEEP => bdwgc finds it
 *   - lost: only in a wasm local              => bdwgc misses it
 * After GC + heap reuse we check the contents of both.
 */
#include <stdio.h>
#include <string.h>
#include <gc.h>

/*
 * Force clang to spill a local to the shadow stack by taking its address
 * in an empty inline-asm block. One i32.store at the site; subsequent
 * uses of the local still come from the wasm local (no reloads).
 */
#define GC_KEEP(x) __asm__ volatile("" :: "r"(&(x)))

#define CHUNK 1024
static const char MAGIC_KEPT[] = "hello-i-am-kept-alive";
static const char MAGIC_LOST[] = "hello-i-will-be-lost!";

static void __attribute__((noinline)) burn_through_heap(void) {
    /* Churn the freelist so freed blocks get reused. */
    for (int i = 0; i < 20000; i++) {
        char *q = GC_MALLOC(CHUNK);
        memset(q, 0xAA, CHUNK);
    }
}

static void __attribute__((noinline)) dump(const char *label, const char *p, size_t n) {
    printf("  %-6s: ", label);
    for (size_t i = 0; i < n; i++)
        printf("%02x ", (unsigned char)p[i]);
    printf("\n");
}

static void __attribute__((noinline))
check_after_gc(char *kept, char *lost) {
    GC_gcollect();
    burn_through_heap();

    const size_t nk = sizeof(MAGIC_KEPT) - 1;
    const size_t nl = sizeof(MAGIC_LOST) - 1;

    printf("kept buffer:\n");
    dump("got",    kept, nk);
    dump("want",   MAGIC_KEPT, nk);
    printf("  => %s\n",
           memcmp(kept, MAGIC_KEPT, nk) == 0 ? "OK (survived)"
                                             : "BUG (reclaimed)");

    printf("lost buffer:\n");
    dump("got",    lost, nl);
    dump("want",   MAGIC_LOST, nl);
    printf("  => %s\n",
           memcmp(lost, MAGIC_LOST, nl) == 0 ? "OK (survived)"
                                             : "BUG (reclaimed)");
}

int main(void) {
    GC_INIT();

    char *kept = GC_MALLOC(CHUNK);
    GC_KEEP(kept);              /* spill to shadow stack */

    char *lost = GC_MALLOC(CHUNK);
    /* no GC_KEEP for lost: it stays only in a wasm local */

    strcpy(kept, MAGIC_KEPT);
    strcpy(lost, MAGIC_LOST);

    check_after_gc(kept, lost);
    return 0;
}
