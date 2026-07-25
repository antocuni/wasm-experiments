/*
 * Test different candidates for a zero-overhead "spill this local"
 * primitive. Each variant calls check_after_gc(p) after arranging
 * (or not) for p to be in addressable memory before the call.
 *
 * Success criterion: after GC + heap reuse, p still points at MAGIC.
 *
 * Compile with `make spill_test.wasm && wasmtime spill_test.wasm`.
 * Inspect the generated wat with `wasm-tools print spill_test.wasm`.
 */
#include <stdio.h>
#include <string.h>
#include <gc.h>

#define CHUNK 1024
static const char MAGIC[] = "hello-from-the-past";

static void __attribute__((noinline)) burn_through_heap(void) {
    for (int i = 0; i < 20000; i++) {
        char *q = GC_MALLOC(CHUNK);
        memset(q, 0xAA, CHUNK);
    }
}

static int __attribute__((noinline)) check_after_gc(char *p) {
    GC_gcollect();
    burn_through_heap();
    return memcmp(p, MAGIC, sizeof(MAGIC) - 1) == 0;
}

/* --- Candidate spill primitives --- */

/*
 * A: empty asm block, just reads &p. No memory clobber. Should force
 * clang to materialize &p (=> spill p to the shadow stack) but leave
 * subsequent uses of p free to come from the wasm local.
 */
#define SPILL_ASM_NOMEM(x) __asm__ volatile("" :: "r"(&(x)))

/*
 * B: same, but with a "memory" clobber. Stronger: also invalidates
 * the register copy so any later read of p reloads from memory. More
 * pessimistic but safer if we don't fully trust A.
 */
#define SPILL_ASM_MEM(x) __asm__ volatile("" :: "r"(&(x)) : "memory")

/*
 * C: noinline empty function taking &p by value. Portable fallback if
 * wasm-clang doesn't like inline asm operand constraints. Costs one
 * empty call.
 */
static void __attribute__((noinline)) gc_spill(void *addr) { (void)addr; }
#define SPILL_CALL(x) gc_spill(&(x))

static void run(const char *label, int ok) {
    printf("[%-14s] %s\n", label, ok ? "OK (survived)" : "BUG (reclaimed)");
}

static void __attribute__((noinline)) test_baseline(void) {
    char *p = GC_MALLOC(CHUNK);
    strcpy(p, MAGIC);
    run("baseline",     check_after_gc(p));
}

static void __attribute__((noinline)) test_asm_nomem(void) {
    char *p = GC_MALLOC(CHUNK);
    SPILL_ASM_NOMEM(p);
    strcpy(p, MAGIC);
    run("asm_nomem",    check_after_gc(p));
}

static void __attribute__((noinline)) test_asm_mem(void) {
    char *p = GC_MALLOC(CHUNK);
    SPILL_ASM_MEM(p);
    strcpy(p, MAGIC);
    run("asm_mem",      check_after_gc(p));
}

static void __attribute__((noinline)) test_call(void) {
    char *p = GC_MALLOC(CHUNK);
    SPILL_CALL(p);
    strcpy(p, MAGIC);
    run("call",         check_after_gc(p));
}

int main(void) {
    GC_INIT();
    test_baseline();
    test_asm_nomem();
    test_asm_mem();
    test_call();
    return 0;
}
