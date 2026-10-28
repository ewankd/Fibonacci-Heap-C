#include "fibonacci_heap.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

/* Checks stay enabled even when the library is compiled with -DNDEBUG. */
#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static FibonacciHeap *new_heap(void) {
    FibonacciHeap *heap = fh_create();
    CHECK(heap != NULL);
    return heap;
}

static void test_empty_and_null_arguments(void) {
    FibonacciHeap *heap = new_heap();
    int marker = 0;
    int64_t key = 123;
    void *value = &marker;
    CHECK(fh_is_empty(heap));
    CHECK(fh_size(heap) == 0);
    CHECK(fh_validate(heap));
    CHECK(!fh_minimum(heap, &key, &value));
    CHECK(!fh_extract_min(heap, &key, &value));
    CHECK(key == 123 && value == &marker);
    CHECK(fh_is_empty(NULL));
    CHECK(!fh_validate(NULL));
    CHECK(!fh_minimum(NULL, NULL, NULL));
    CHECK(!fh_extract_min(NULL, NULL, NULL));
    CHECK(fh_insert(NULL, 1, NULL) == NULL);
    CHECK(!fh_decrease_key(heap, NULL, 1));
    CHECK(!fh_delete(heap, NULL, NULL));
    CHECK(!fh_meld(heap, heap));
    CHECK(!fh_meld(heap, NULL));
    CHECK(!fh_meld(NULL, heap));
    fh_destroy(NULL);
    fh_destroy(heap);
}

static void test_singleton_and_borrowed_payload(void) {
    FibonacciHeap *heap = new_heap();
    int payload = 77;
    FibonacciNode *node = fh_insert(heap, 9, &payload);
    CHECK(node != NULL);
    CHECK(fh_node_key(node) == 9 && fh_node_value(node) == &payload);
    CHECK(fh_node_degree(node) == 0 && fh_node_parent(node) == NULL);
    CHECK(!fh_node_is_marked(node));
    int64_t key;
    void *value;
    CHECK(fh_minimum(heap, &key, &value));
    CHECK(key == 9 && value == &payload);
    CHECK(fh_decrease_key(heap, node, 9));
    CHECK(!fh_decrease_key(heap, node, 10));
    CHECK(fh_node_key(node) == 9);
    CHECK(fh_decrease_key(heap, node, -2));
    CHECK(fh_extract_min(heap, &key, &value));
    CHECK(key == -2 && value == &payload && payload == 77);
    CHECK(fh_validate(heap) && fh_is_empty(heap));
    fh_destroy(heap);
    CHECK(payload == 77);
}

static void test_sort_and_consolidation(void) {
    FibonacciHeap *heap = new_heap();
    const int count = 5000;
    for (int i = count - 1; i >= 0; i--) {
        CHECK(fh_insert(heap, i, NULL) != NULL);
    }
    CHECK(fh_size(heap) == (size_t)count && fh_validate(heap));
    for (int i = 0; i < count; i++) {
        int64_t key;
        CHECK(fh_extract_min(heap, &key, NULL) && key == i);
        if (i % 100 == 0) {
            CHECK(fh_validate(heap));
        }
    }
    CHECK(fh_is_empty(heap) && fh_validate(heap));
    fh_destroy(heap);
}

static void test_duplicates_extreme_keys_and_exact_deletion(void) {
    FibonacciHeap *heap = new_heap();
    int payloads[] = {1, 2, 3, 4, 5};
    CHECK(fh_insert(heap, INT64_MIN, &payloads[0]) != NULL);
    FibonacciNode *second_min = fh_insert(heap, INT64_MIN, &payloads[1]);
    FibonacciNode *maximum = fh_insert(heap, INT64_MAX, &payloads[2]);
    CHECK(second_min != NULL && maximum != NULL);
    CHECK(fh_insert(heap, 7, &payloads[3]) != NULL);
    CHECK(fh_insert(heap, 7, &payloads[4]) != NULL);
    void *value;
    CHECK(fh_delete(heap, second_min, &value));
    CHECK(value == &payloads[1]);
    CHECK(fh_validate(heap));
    int64_t key;
    CHECK(fh_extract_min(heap, &key, &value));
    CHECK(key == INT64_MIN && value == &payloads[0]);
    CHECK(fh_decrease_key(heap, maximum, INT64_MIN));
    CHECK(fh_extract_min(heap, &key, &value));
    CHECK(key == INT64_MIN && value == &payloads[2]);
    CHECK(fh_extract_min(heap, &key, NULL) && key == 7);
    CHECK(fh_extract_min(heap, &key, NULL) && key == 7);
    CHECK(fh_is_empty(heap) && fh_validate(heap));
    fh_destroy(heap);
}

static void test_meld_handle_transfer_and_source_reuse(void) {
    FibonacciHeap *a = new_heap(), *b = new_heap();
    CHECK(fh_insert(a, -3, NULL) != NULL);
    FibonacciNode *handle = NULL;
    for (int i = 0; i < 64; i++) {
        FibonacciNode *node = fh_insert(b, i, NULL);
        CHECK(node != NULL);
        if (i == 50) {
            handle = node;
        }
    }
    CHECK(fh_extract_min(b, NULL, NULL));
    CHECK(fh_validate(b));
    CHECK(fh_meld(a, b));
    CHECK(fh_size(a) == 64 && fh_is_empty(b));
    CHECK(fh_validate(a) && fh_validate(b));
    CHECK(fh_decrease_key(a, handle, -9));
    CHECK(fh_insert(b, -100, NULL) != NULL);
    fh_destroy(b);  /* transferred nodes must survive destruction of source */
    int64_t key;
    CHECK(fh_extract_min(a, &key, NULL) && key == -9);
    CHECK(fh_validate(a));
    fh_destroy(a);
}

static void test_empty_meld_combinations(void) {
    FibonacciHeap *a = new_heap(), *b = new_heap();
    CHECK(fh_meld(a, b) && fh_validate(a) && fh_validate(b));
    CHECK(fh_insert(b, 4, NULL) != NULL);
    CHECK(fh_meld(a, b));
    CHECK(fh_size(a) == 1 && fh_is_empty(b));
    CHECK(fh_meld(a, b));
    CHECK(fh_meld(b, a));
    CHECK(fh_size(b) == 1 && fh_is_empty(a));
    CHECK(fh_validate(a) && fh_validate(b));
    fh_destroy(a);
    fh_destroy(b);
}

static void test_equal_parent_key_does_not_cut(void) {
    FibonacciHeap *heap = new_heap();
    FibonacciNode *nodes[32];
    for (int i = 0; i < 32; i++) {
        nodes[i] = fh_insert(heap, i, NULL);
        CHECK(nodes[i] != NULL);
    }
    CHECK(fh_extract_min(heap, NULL, NULL));
    FibonacciNode *child = NULL;
    for (int i = 1; i < 32; i++) {
        if (fh_node_parent(nodes[i]) != NULL) {
            child = nodes[i];
            break;
        }
    }
    CHECK(child != NULL);
    const FibonacciNode *parent = fh_node_parent(child);
    CHECK(fh_decrease_key(heap, child, fh_node_key(parent)));
    CHECK(fh_node_parent(child) == parent);
    CHECK(fh_decrease_key(heap, child, fh_node_key(parent) - 1));
    CHECK(fh_node_parent(child) == NULL && !fh_node_is_marked(child));
    CHECK(fh_validate(heap));
    fh_destroy(heap);
}

static void test_cascading_cut_after_second_child_loss(void) {
    FibonacciHeap *heap = new_heap();
    FibonacciNode *nodes[256];
    for (int i = 0; i < 256; i++) {
        nodes[i] = fh_insert(heap, i, NULL);
        CHECK(nodes[i] != NULL);
    }
    CHECK(fh_insert(heap, -1, NULL) != NULL);
    CHECK(fh_extract_min(heap, NULL, NULL)); /* consolidate 256 remaining nodes */
    FibonacciNode *parent = NULL;
    for (int i = 0; i < 256; i++) {
        if (fh_node_parent(nodes[i]) != NULL && fh_node_degree(nodes[i]) >= 2) {
            parent = nodes[i];
            break;
        }
    }
    CHECK(parent != NULL && !fh_node_is_marked(parent));
    FibonacciNode *children[2] = {NULL, NULL};
    size_t found = 0;
    for (int i = 0; i < 256 && found < 2; i++) {
        if (fh_node_parent(nodes[i]) == parent) {
            children[found++] = nodes[i];
        }
    }
    CHECK(found == 2);
    const FibonacciNode *grandparent = fh_node_parent(parent);
    CHECK(fh_decrease_key(heap, children[0], -1000));
    CHECK(fh_node_parent(parent) == grandparent && fh_node_is_marked(parent));
    CHECK(fh_validate(heap));
    CHECK(fh_decrease_key(heap, children[1], -2000));
    CHECK(fh_node_parent(parent) == NULL && !fh_node_is_marked(parent));
    CHECK(fh_node_parent(children[0]) == NULL && fh_node_parent(children[1]) == NULL);
    CHECK(fh_validate(heap));
    fh_destroy(heap);
}

static void test_delete_internal_node_with_children(void) {
    FibonacciHeap *heap = new_heap();
    FibonacciNode *nodes[128];
    for (int i = 0; i < 128; i++) {
        nodes[i] = fh_insert(heap, i, NULL);
        CHECK(nodes[i] != NULL);
    }
    CHECK(fh_extract_min(heap, NULL, NULL));
    int removed = -1;
    for (int i = 1; i < 128; i++) {
        if (fh_node_parent(nodes[i]) != NULL && fh_node_degree(nodes[i]) > 0) {
            removed = i;
            CHECK(fh_delete(heap, nodes[i], NULL));
            break;
        }
    }
    CHECK(removed != -1 && fh_validate(heap));
    for (int i = 1; i < 128; i++) {
        if (i != removed) {
            int64_t key;
            CHECK(fh_extract_min(heap, &key, NULL) && key == i);
        }
    }
    CHECK(fh_validate(heap));
    fh_destroy(heap);
}

static uint32_t next_random(uint32_t *state) {
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
}

typedef struct {
    FibonacciNode *node;
    int64_t key;
    bool alive;
} ReferenceEntry;

static void test_randomized_against_reference_queue(void) {
    enum {CAPACITY = 400, STEPS = 10000};
    for (uint32_t seed = 1; seed <= 3; seed++) {
        FibonacciHeap *heap = new_heap();
        ReferenceEntry entries[CAPACITY] = {0};
        size_t live = 0;
        uint32_t state = seed;
        for (int step = 0; step < STEPS; step++) {
            size_t slot = next_random(&state) % CAPACITY;
            unsigned operation = next_random(&state) % 4U;
            ReferenceEntry *entry = &entries[slot];
            if (operation == 0 && !entry->alive) {
                entry->key = (int64_t)(next_random(&state) % 2001U) - 1000;
                entry->node = fh_insert(heap, entry->key, entry);
                CHECK(entry->node != NULL);
                entry->alive = true;
                live++;
            } else if (operation == 1 && entry->alive) {
                entry->key -= (int64_t)(next_random(&state) % 100U);
                CHECK(fh_decrease_key(heap, entry->node, entry->key));
            } else if (operation == 2 && entry->alive) {
                void *value;
                CHECK(fh_delete(heap, entry->node, &value) && value == entry);
                entry->alive = false;
                entry->node = NULL;
                live--;
            } else if (operation == 3 && live > 0) {
                int64_t expected = INT64_MAX;
                for (size_t i = 0; i < CAPACITY; i++) {
                    if (entries[i].alive && entries[i].key < expected) {
                        expected = entries[i].key;
                    }
                }
                int64_t actual;
                void *value;
                CHECK(fh_extract_min(heap, &actual, &value) && actual == expected);
                ReferenceEntry *extracted = value;
                CHECK(extracted != NULL && extracted->alive && extracted->key == actual);
                extracted->alive = false;
                extracted->node = NULL;
                live--;
            }
            CHECK(fh_size(heap) == live && fh_validate(heap));
            int64_t actual;
            if (live == 0) {
                CHECK(!fh_minimum(heap, &actual, NULL));
            } else {
                int64_t expected = INT64_MAX;
                for (size_t i = 0; i < CAPACITY; i++) {
                    if (entries[i].alive && entries[i].key < expected) {
                        expected = entries[i].key;
                    }
                }
                CHECK(fh_minimum(heap, &actual, NULL) && actual == expected);
            }
        }
        fh_destroy(heap);
    }
}

static void test_destroy_nonempty_forests(void) {
    FibonacciHeap *heap = new_heap();
    for (int i = 0; i < 20000; i++) {
        CHECK(fh_insert(heap, i, NULL) != NULL);
    }
    CHECK(fh_extract_min(heap, NULL, NULL));
    for (int i = -1000; i < 0; i++) {
        CHECK(fh_insert(heap, i, NULL) != NULL);
    }
    CHECK(fh_validate(heap));
    fh_destroy(heap); /* sanitizers check the remaining trees for leaks */
}

static void run_test(const char *name, void (*test)(void)) {
    test();
    printf("PASS %s\n", name);
}

int main(void) {
    run_test("empty and NULL arguments", test_empty_and_null_arguments);
    run_test("singleton and borrowed payload", test_singleton_and_borrowed_payload);
    run_test("sorted extraction and consolidation", test_sort_and_consolidation);
    run_test("duplicates, extreme keys and exact deletion", test_duplicates_extreme_keys_and_exact_deletion);
    run_test("meld transfers handles; source reuse", test_meld_handle_transfer_and_source_reuse);
    run_test("empty meld combinations", test_empty_meld_combinations);
    run_test("equal-parent boundary and cut", test_equal_parent_key_does_not_cut);
    run_test("cascading cut after second child loss", test_cascading_cut_after_second_child_loss);
    run_test("delete internal node with children", test_delete_internal_node_with_children);
    run_test("30,000 seeded reference operations", test_randomized_against_reference_queue);
    run_test("destroy nonempty forests", test_destroy_nonempty_forests);
    puts("All 11 test groups passed.");
    return EXIT_SUCCESS;
}
