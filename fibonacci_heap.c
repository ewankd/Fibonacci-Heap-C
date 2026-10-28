#include "fibonacci_heap.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>

struct FibonacciNode {
    int64_t key;
    void *value;
    struct FibonacciNode *parent;
    struct FibonacciNode *child;
    struct FibonacciNode *left;
    struct FibonacciNode *right;
    size_t degree;
    bool marked;
};

struct FibonacciHeap {
    FibonacciNode *minimum;
    size_t size;
};

/* A valid n-node Fibonacci heap has maximum degree O(log_phi(n)).
 * Since log_phi(n) < 2*log_2(n), this table covers any size_t-sized heap.
 * It avoids allocation failure while consolidating after extraction.
 */
#define DEGREE_SLOTS (2 * sizeof(size_t) * CHAR_BIT + 1)

static void ring_remove(FibonacciNode *node) {
    node->left->right = node->right;
    node->right->left = node->left;
    node->left = node;
    node->right = node;
}

static void ring_insert_after(FibonacciNode *anchor, FibonacciNode *node) {
    node->right = anchor->right;
    node->left = anchor;
    anchor->right->left = node;
    anchor->right = node;
}

/* Join two disjoint, nonempty circular doubly linked lists in O(1). */
static void ring_join(FibonacciNode *a, FibonacciNode *b) {
    FibonacciNode *a_next = a->right;
    FibonacciNode *b_last = b->left;
    a->right = b;
    b->left = a;
    b_last->right = a_next;
    a_next->left = b_last;
}

static void add_root(FibonacciHeap *heap, FibonacciNode *node) {
    node->parent = NULL;
    node->marked = false;
    if (heap->minimum == NULL) {
        node->left = node;
        node->right = node;
        heap->minimum = node;
    } else {
        ring_insert_after(heap->minimum, node);
        if (node->key < heap->minimum->key) {
            heap->minimum = node;
        }
    }
}

/* Both arguments are detached roots; parent wins the key comparison. */
static void link_child(FibonacciNode *parent, FibonacciNode *child) {
    child->parent = parent;
    child->marked = false;
    if (parent->child == NULL) {
        parent->child = child;
    } else {
        ring_insert_after(parent->child, child);
    }
    parent->degree++;
}

static void consolidate(FibonacciHeap *heap) {
    FibonacciNode *by_degree[DEGREE_SLOTS] = {NULL};

    /* Remove each unprocessed root before linking it. Thus, linking an
     * already processed root never invalidates the unprocessed root ring.
     */
    while (heap->minimum != NULL) {
        FibonacciNode *root = heap->minimum;
        heap->minimum = root->right == root ? NULL : root->right;
        ring_remove(root);
        size_t degree = root->degree;
        assert(degree < DEGREE_SLOTS);
        while (by_degree[degree] != NULL) {
            FibonacciNode *other = by_degree[degree];
            by_degree[degree] = NULL;
            if (other->key < root->key) {
                FibonacciNode *temporary = root;
                root = other;
                other = temporary;
            }
            link_child(root, other);
            degree = root->degree;
            assert(degree < DEGREE_SLOTS);
        }
        by_degree[degree] = root;
    }

    for (size_t degree = 0; degree < DEGREE_SLOTS; degree++) {
        if (by_degree[degree] != NULL) {
            add_root(heap, by_degree[degree]);
        }
    }
}

static void cut(FibonacciHeap *heap, FibonacciNode *node, FibonacciNode *parent) {
    if (parent->child == node) {
        parent->child = node->right == node ? NULL : node->right;
    }
    parent->degree--;
    ring_remove(node);
    add_root(heap, node);
}

static void cascading_cut(FibonacciHeap *heap, FibonacciNode *node) {
    FibonacciNode *parent = node->parent;
    while (parent != NULL) {
        if (!node->marked) {
            node->marked = true;
            return;
        }
        cut(heap, node, parent);
        node = parent;
        parent = node->parent;
    }
}

FibonacciHeap *fh_create(void) {
    return calloc(1, sizeof(FibonacciHeap));
}

void fh_destroy(FibonacciHeap *heap) {
    if (heap == NULL) {
        return;
    }
    /* Flatten child rings into the pending work ring. No recursion, auxiliary
     * allocation or consolidation is needed; every node is freed once.
     */
    while (heap->minimum != NULL) {
        FibonacciNode *node = heap->minimum;
        if (node->child != NULL) {
            ring_join(node, node->child);
        }
        heap->minimum = node->right == node ? NULL : node->right;
        ring_remove(node);
        free(node);
    }
    free(heap);
}

size_t fh_size(const FibonacciHeap *heap) {
    return heap == NULL ? 0 : heap->size;
}

bool fh_is_empty(const FibonacciHeap *heap) {
    return fh_size(heap) == 0;
}

FibonacciNode *fh_insert(FibonacciHeap *heap, int64_t key, void *value) {
    if (heap == NULL || heap->size == SIZE_MAX) {
        return NULL;
    }
    FibonacciNode *node = calloc(1, sizeof(FibonacciNode));
    if (node == NULL) {
        return NULL;
    }
    node->key = key;
    node->value = value;
    node->left = node;
    node->right = node;
    add_root(heap, node);
    heap->size++;
    return node;
}

bool fh_minimum(const FibonacciHeap *heap, int64_t *key, void **value) {
    if (heap == NULL || heap->minimum == NULL) {
        return false;
    }
    if (key != NULL) {
        *key = heap->minimum->key;
    }
    if (value != NULL) {
        *value = heap->minimum->value;
    }
    return true;
}

bool fh_extract_min(FibonacciHeap *heap, int64_t *key, void **value) {
    if (heap == NULL || heap->minimum == NULL) {
        return false;
    }
    FibonacciNode *removed = heap->minimum;
    while (removed->child != NULL) {
        FibonacciNode *child = removed->child;
        removed->child = child->right == child ? NULL : child->right;
        ring_remove(child);
        removed->degree--;
        add_root(heap, child);
    }

    heap->minimum = removed->right == removed ? NULL : removed->right;
    ring_remove(removed);
    heap->size--;
    if (heap->minimum != NULL) {
        consolidate(heap);
    }
    if (key != NULL) {
        *key = removed->key;
    }
    if (value != NULL) {
        *value = removed->value;
    }
    free(removed);
    return true;
}

bool fh_decrease_key(FibonacciHeap *heap, FibonacciNode *node, int64_t new_key) {
    if (heap == NULL || node == NULL || heap->minimum == NULL || new_key > node->key) {
        return false;
    }
    node->key = new_key;
    FibonacciNode *parent = node->parent;
    if (parent != NULL && node->key < parent->key) {
        cut(heap, node, parent);
        cascading_cut(heap, parent);
    }
    if (node->key < heap->minimum->key) {
        heap->minimum = node;
    }
    return true;
}

bool fh_delete(FibonacciHeap *heap, FibonacciNode *node, void **value) {
    if (heap == NULL || node == NULL || heap->minimum == NULL) {
        return false;
    }
    FibonacciNode *parent = node->parent;
    if (parent != NULL) {
        cut(heap, node, parent);
        cascading_cut(heap, parent);
    }
    /* Select this root for removal directly. This avoids a numeric sentinel
     * and deletes the requested handle even if INT64_MIN already exists.
     * Extraction consolidates all remaining roots and recomputes the minimum.
     */
    heap->minimum = node;
    return fh_extract_min(heap, NULL, value);
}

bool fh_meld(FibonacciHeap *destination, FibonacciHeap *source) {
    if (destination == NULL || source == NULL || destination == source ||
        destination->size > SIZE_MAX - source->size) {
        return false;
    }
    if (source->minimum == NULL) {
        return true;
    }
    if (destination->minimum == NULL) {
        destination->minimum = source->minimum;
    } else {
        ring_join(destination->minimum, source->minimum);
        if (source->minimum->key < destination->minimum->key) {
            destination->minimum = source->minimum;
        }
    }
    destination->size += source->size;
    source->minimum = NULL;
    source->size = 0;
    return true;
}

int64_t fh_node_key(const FibonacciNode *node) {
    assert(node != NULL);
    return node->key;
}

void *fh_node_value(const FibonacciNode *node) {
    return node == NULL ? NULL : node->value;
}

const FibonacciNode *fh_node_parent(const FibonacciNode *node) {
    return node == NULL ? NULL : node->parent;
}

size_t fh_node_degree(const FibonacciNode *node) {
    return node == NULL ? 0 : node->degree;
}

bool fh_node_is_marked(const FibonacciNode *node) {
    return node != NULL && node->marked;
}

/* Validate one sibling ring and queue its members for iterative traversal. */
static bool queue_ring(const FibonacciHeap *heap, const FibonacciNode *start,
                       const FibonacciNode *parent, const FibonacciNode **stack,
                       size_t *top, size_t *count) {
    *count = 0;
    if (start == NULL) {
        return true;
    }
    const FibonacciNode *node = start;
    do {
        if (node == NULL || node->left == NULL || node->right == NULL ||
            node->left->right != node || node->right->left != node ||
            node->parent != parent || *count >= heap->size || *top >= heap->size ||
            node->key < heap->minimum->key) {
            return false;
        }
        if ((parent != NULL && node->key < parent->key) ||
            (parent == NULL && node->marked)) {
            return false;
        }
        stack[(*top)++] = node;
        (*count)++;
        node = node->right;
    } while (node != start);
    return true;
}

bool fh_validate(const FibonacciHeap *heap) {
    if (heap == NULL) {
        return false;
    }
    if (heap->size == 0) {
        return heap->minimum == NULL;
    }
    if (heap->minimum == NULL || heap->size > SIZE_MAX / sizeof(FibonacciNode *)) {
        return false;
    }
    const FibonacciNode **stack = malloc(heap->size * sizeof(*stack));
    if (stack == NULL) {
        return false;
    }
    size_t top = 0, roots = 0, visited = 0;
    bool valid = queue_ring(heap, heap->minimum, NULL, stack, &top, &roots);
    while (valid && top > 0) {
        const FibonacciNode *node = stack[--top];
        if (++visited > heap->size) {
            valid = false;
            break;
        }
        size_t children = 0;
        valid = queue_ring(heap, node->child, node, stack, &top, &children);
        if (children != node->degree) {
            valid = false;
        }
    }
    free(stack);
    return valid && visited == heap->size;
}
