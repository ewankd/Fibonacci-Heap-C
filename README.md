# Fibonacci Heap — C

A C11 min-priority queue implemented from scratch using circular doubly linked root/child lists, lazy consolidation, node marks and cascading cuts. Supports duplicate signed 64-bit keys, stable handles, borrowed payloads, destructive meld and deletion of a specific handle.

## Build and run

Requires a C11 compiler such as GCC or Clang. No third-party runtime libraries are needed.

### Linux / macOS (Make)

From this project directory:

```bash
make demo
make test
make sanitize
```

`make sanitize` requires a compiler/toolchain with AddressSanitizer, UndefinedBehaviorSanitizer and leak detection support. The normal demo/tests do not require sanitizers. `make clean` removes build output.

Some restricted containers prevent LeakSanitizer from inspecting processes. There, use `make sanitize ASAN_OPTIONS=detect_leaks=0` for address/undefined-behavior checks. On systems with a GNU-compatible linker, `make memory` additionally counts outstanding library allocations and injects allocation failures, without relying on process inspection.

### Windows or any platform without Make

With GCC/Clang available on PATH (for example, GCC in MSYS2 on Windows), run these commands from the project directory:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -Iinclude src/fibonacci_heap.c examples/demo.c -o demo.exe
gcc -std=c11 -Wall -Wextra -Wpedantic -Iinclude src/fibonacci_heap.c tests/test_heap.c -o test_heap.exe
```

In Windows PowerShell, run `./demo.exe` and `./test_heap.exe`. On Linux/macOS you can use `cc` instead of `gcc`, and run the same output paths. A compiler must be installed before building; this ZIP contains source, not platform-specific executables.

## Demonstration

`make demo` inserts priorities 23, 7, 21, 3, 18 and 39; melds a second queue containing 12; decreases Job F from 39 to 1; then extracts priorities in this order:

```text
1, 3, 7, 12, 18, 21, 23
```

## Library example

```c
#include "fibonacci_heap.h"
#include <stdio.h>

int main(void) {
    FibonacciHeap *heap = fh_create();
    if (heap == NULL) return 1;
    int task_id = 42;
    FibonacciNode *task = fh_insert(heap, 10, &task_id);
    if (task == NULL) { fh_destroy(heap); return 1; }
    if (!fh_decrease_key(heap, task, 3)) { fh_destroy(heap); return 1; }
    int64_t priority;
    void *payload;
    if (fh_extract_min(heap, &priority, &payload)) {
        printf("Task %d is ready.\n", *(int *)payload);
    }
    /* task is now invalid: extraction freed its node. */
    fh_destroy(heap);
    return 0;
}
```

Compile a program using the library by including `include/` and linking `src/fibonacci_heap.c` as shown in the build commands.

## Operations and complexity

Let n be the number of live nodes. The standard Fibonacci-heap bounds below assume constant-time fixed-width integer comparisons, pointer operations and allocator calls. **Amortized bounds apply across a sequence, not to every individual call.** Extract-min or a chain of cascading cuts can take linear actual time in one call.

| API | Behavior | Time bound |
| --- | --- | --- |
| `fh_create()` | Allocate an empty heap | O(1) |
| `fh_insert(heap, key, value)` | Return a node handle, or NULL on failure | O(1) amortized |
| `fh_minimum(heap, &key, &value)` | Read minimum; false if empty | O(1) |
| `fh_extract_min(heap, &key, &value)` | Remove/free a minimum node | O(log n) amortized |
| `fh_decrease_key(heap, node, new_key)` | Reject increases; cut/cascade when necessary | O(1) amortized |
| `fh_delete(heap, node, &value)` | Remove/free exactly the requested node | O(log n) amortized |
| `fh_meld(destination, source)` | Transfer all nodes; leave source empty/reusable | O(1) |
| `fh_size(heap)`, `fh_is_empty(heap)` | Read cached size | O(1) |
| `fh_node_*` | Read a live node's key, payload, parent, degree or mark | O(1) |
| `fh_validate(heap)` | Check links, parent pointers, heap ordering, degrees, marks, count and minimum | O(n) time and auxiliary space |
| `fh_destroy(heap)` | Free all nodes and the heap | O(n) time; O(1) auxiliary space |

The core heap occupies O(n) memory. The theoretical degree bound is O(log n): a degree-d node contains at least a Fibonacci-number-sized subtree. Consolidation uses a fixed stack table with `2 * sizeof(size_t) * CHAR_BIT + 1` slots, covering the degree bound for any representable heap size and avoiding allocation during extract-min. `fh_validate` separately allocates traversal scratch space and can return false if that allocation fails.

## How it works

1. **Insertion:** add a singleton tree to the root ring and update the minimum pointer.
2. **Extract minimum:** promote its children, remove/free the selected root, repeatedly link equal-degree roots, and rebuild the root ring with a new minimum.
3. **Decrease key:** cut a child whose key becomes strictly smaller than its parent's. Mark a non-root parent after its first child loss; cut it after its second loss and continue upward.
4. **Meld:** splice root rings in constant time and transfer the source's count and node ownership.
5. **Delete:** cut the requested node to the root list, select that root for removal, then consolidate. This avoids a sentinel-key trick and handles duplicate `INT64_MIN` priorities correctly.

The usual amortized analysis uses potential **number of roots + 2 × number of marked nodes**. Insertions accumulate potential; consolidation and cascading cuts spend it. The implementation is a Fibonacci heap, not a binary heap wrapper.

## Handle and memory contract

- Keys may repeat. Equal-priority extraction order is unspecified.
- A handle stays at the same address through consolidation, decreases and meld.
- A handle passed to decrease/delete must be live and belong to that heap. After a successful meld, handles from the source belong to the destination. Ownership is a caller precondition; wrong-heap or stale-handle misuse is not detected.
- Extract-min and delete free the removed node. Never use its handle afterward. Destroy invalidates every remaining node handle and the heap pointer.
- Payload pointers are borrowed. The caller owns their lifetime; the heap never frees or copies payload data. Output key/value pointers can be NULL when their values are not needed.
- `fh_insert`/`fh_create` report allocation failures with NULL. Invalid NULL, self-meld, key-increase and size-overflow operations fail without mutation as documented in the header. `fh_node_key` requires a non-NULL live handle. `fh_size(NULL)` is 0, `fh_is_empty(NULL)` is true, and `fh_validate(NULL)` is false.
- The library is not thread-safe. The validator checks its own readable internal structure; it is not a memory-safety tool for arbitrary corrupted pointers. Sanitizers serve that purpose during testing.

## Tests

Eleven named test groups cover empty/singleton behavior; borrowed payloads; 5,000 sorted extractions; duplicate/extreme keys; deletion of the exact requested handle; meld and source reuse; strict parent-key comparison; a deterministic second-child-loss cascading cut; deletion of internal nodes with children; 30,000 seeded mixed operations checked against a reference priority queue; and destruction of a nonempty forest.

The randomized tests validate invariants after every operation and compare size/minimum against the reference. Test checks remain enabled under `-DNDEBUG`. `make sanitize` runs the suite with address/undefined-behavior checking and requests leak detection. These are tests, not a claim of formal verification or measured superiority over binary heaps.

`make memory` verifies failed heap/node/scratch allocations leave state intact, extraction/decrease/delete/meld/destruction require no allocations, and the tested lifecycle ends with zero outstanding library allocations. It uses optional GNU-linker wrapping and is separate from the portable C11 implementation.

## Project layout

- `include/fibonacci_heap.h`: public API, return values and ownership rules.
- `src/fibonacci_heap.c`: implementation.
- `examples/demo.c`: runnable priority queue example.
- `tests/test_heap.c`: deterministic and randomized tests.
- `tests/allocation_checks.c`: optional allocation accounting/failure injection.
- `Makefile`: normal, test, sanitizer and cleanup targets.
- `.gitignore`: excludes binaries and build output.

## Algorithm reference

[MIT 6.854: Fibonacci Heaps, David Karger](https://courses.csail.mit.edu/6.854/21/Notes/n01-fibonacci.html) explains the heap-ordered forest, lazy consolidation, cascading cuts and amortized analysis. This project contains a new educational implementation, not copied course source code.

Development was AI-assisted. The project has no AI models or runtime AI services.
