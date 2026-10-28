/* Optional GNU-linker wrappers: count outstanding allocations and inject
 * failures without changing the library's production implementation.
 */
#include "fibonacci_heap.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void __real_free(void *pointer);

static size_t live_allocations = 0;
static size_t attempts = 0;
static size_t fail_at = SIZE_MAX;

void *__wrap_malloc(size_t size) {
    if (attempts++ == fail_at) {
        return NULL;
    }
    void *pointer = __real_malloc(size);
    if (pointer != NULL) {
        live_allocations++;
    }
    return pointer;
}

void *__wrap_calloc(size_t count, size_t size) {
    if (attempts++ == fail_at) {
        return NULL;
    }
    void *pointer = __real_calloc(count, size);
    if (pointer != NULL) {
        live_allocations++;
    }
    return pointer;
}

void __wrap_free(void *pointer) {
    if (pointer != NULL) {
        CHECK(live_allocations > 0);
        live_allocations--;
    }
    __real_free(pointer);
}

int main(void) {
    fail_at = attempts;
    CHECK(fh_create() == NULL);
    CHECK(live_allocations == 0);

    FibonacciHeap *a = fh_create(), *b = fh_create();
    CHECK(a != NULL && b != NULL);
    FibonacciNode *handle = fh_insert(a, 100, NULL);
    CHECK(handle != NULL);
    fail_at = attempts;
    CHECK(fh_insert(a, -1, NULL) == NULL);
    CHECK(fh_size(a) == 1 && fh_node_key(handle) == 100);
    CHECK(fh_validate(a));

    fail_at = attempts;
    CHECK(!fh_validate(a)); /* diagnostic scratch allocation may fail */
    CHECK(fh_size(a) == 1 && fh_node_key(handle) == 100);
    CHECK(fh_validate(a));

    for (int i = 0; i < 1000; i++) {
        CHECK(fh_insert(b, i, NULL) != NULL);
    }
    size_t before = attempts;
    fail_at = attempts; /* these core operations must not allocate */
    CHECK(fh_extract_min(b, NULL, NULL));
    CHECK(fh_meld(a, b));
    CHECK(fh_decrease_key(a, handle, -5));
    CHECK(fh_delete(a, handle, NULL));
    for (int i = 0; i < 100; i++) {
        CHECK(fh_extract_min(a, NULL, NULL));
    }
    fh_destroy(a);
    fh_destroy(b);
    CHECK(attempts == before);
    CHECK(live_allocations == 0);
    puts("PASS allocation failure handling, allocation-free core operations and zero outstanding allocations");
    return EXIT_SUCCESS;
}
