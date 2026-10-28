#ifndef FIBONACCI_HEAP_H
#define FIBONACCI_HEAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Opaque heap and node handles. Keys are signed 64-bit integers; duplicates
 * are supported. Payload pointers are borrowed and are never freed here.
 *
 * HANDLE CONTRACT: a node argument must be a live node belonging to the
 * supplied heap. Extraction/deletion frees that node and invalidates its
 * handle. Destroy invalidates every remaining handle. Meld transfers source
 * handles to the destination; the source becomes empty and reusable.
 * Ownership/stale-handle misuse is not detected. This is not thread-safe.
 */
typedef struct FibonacciHeap FibonacciHeap;
typedef struct FibonacciNode FibonacciNode;

/* Returns NULL on allocation failure. destroy(NULL) is harmless. */
FibonacciHeap *fh_create(void);
void fh_destroy(FibonacciHeap *heap);
size_t fh_size(const FibonacciHeap *heap);
bool fh_is_empty(const FibonacciHeap *heap);

/* Returns a stable handle, or NULL on allocation/size-overflow failure. */
FibonacciNode *fh_insert(FibonacciHeap *heap, int64_t key, void *value);

/* Output pointers are optional; empty/NULL heaps return false unchanged. */
bool fh_minimum(const FibonacciHeap *heap, int64_t *key, void **value);
bool fh_extract_min(FibonacciHeap *heap, int64_t *key, void **value);

/* Rejects increased keys. Equal keys are accepted without changing order. */
bool fh_decrease_key(FibonacciHeap *heap, FibonacciNode *node, int64_t new_key);

/* Deletes exactly this node, even with duplicate INT64_MIN keys. */
bool fh_delete(FibonacciHeap *heap, FibonacciNode *node, void **value);

/* Destructive O(1) meld. Rejects NULL, self-meld and size overflow. */
bool fh_meld(FibonacciHeap *destination, FibonacciHeap *source);

/* Read-only inspection. node_key requires a non-NULL live handle. */
int64_t fh_node_key(const FibonacciNode *node);
void *fh_node_value(const FibonacciNode *node);
const FibonacciNode *fh_node_parent(const FibonacciNode *node);
size_t fh_node_degree(const FibonacciNode *node);
bool fh_node_is_marked(const FibonacciNode *node);

/* Diagnostic: O(n) time/space, returns false on invariant/allocation failure.
 * It checks internal structure, not arbitrary invalid/dangling addresses.
 * NULL is not a valid heap. Does not mutate the heap.
 */
bool fh_validate(const FibonacciHeap *heap);

#endif
