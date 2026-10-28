#include "fibonacci_heap.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    FibonacciHeap *jobs = fh_create();
    FibonacciHeap *incoming = fh_create();
    if (jobs == NULL || incoming == NULL) {
        fprintf(stderr, "Unable to allocate heaps.\n");
        fh_destroy(jobs);
        fh_destroy(incoming);
        return EXIT_FAILURE;
    }
    const int64_t keys[] = {23, 7, 21, 3, 18, 39};
    const char *labels[] = {"Job A", "Job B", "Job C", "Job D", "Job E", "Job F"};
    FibonacciNode *urgent = NULL;
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        FibonacciNode *node = fh_insert(jobs, keys[i], (void *)labels[i]);
        if (node == NULL) {
            fprintf(stderr, "Unable to allocate a node.\n");
            fh_destroy(jobs);
            fh_destroy(incoming);
            return EXIT_FAILURE;
        }
        if (i == 5) {
            urgent = node;
        }
    }
    if (fh_insert(incoming, 12, (void *)"Job G") == NULL ||
        !fh_meld(jobs, incoming) || !fh_decrease_key(jobs, urgent, 1)) {
        fprintf(stderr, "Unable to prepare demonstration.\n");
        fh_destroy(jobs);
        fh_destroy(incoming);
        return EXIT_FAILURE;
    }
    puts("FIBONACCI HEAP | C11");
    puts("Inserted priorities: 23, 7, 21, 3, 18, 39");
    puts("Melded another heap with priority 12; decreased Job F from 39 to 1.");
    printf("Size: %zu | Source is empty: %s | Valid: %s\n", fh_size(jobs),
           fh_is_empty(incoming) ? "yes" : "no", fh_validate(jobs) ? "yes" : "no");
    puts("Extracting jobs in priority order:");
    int64_t priority;
    void *value;
    while (fh_extract_min(jobs, &priority, &value)) {
        printf("  %s: %" PRId64 "\n", (const char *)value, priority);
    }
    fh_destroy(incoming);
    fh_destroy(jobs);
    return EXIT_SUCCESS;
}
