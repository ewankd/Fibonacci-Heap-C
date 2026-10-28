CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
CPPFLAGS = -Iinclude
ASAN_OPTIONS ?= detect_leaks=1
BUILD = build
SOURCE = src/fibonacci_heap.c
HEADER = include/fibonacci_heap.h

.PHONY: all demo test sanitize memory clean
all: $(BUILD)/demo $(BUILD)/test_heap

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/demo: $(SOURCE) $(HEADER) examples/demo.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCE) examples/demo.c -o $@

$(BUILD)/test_heap: $(SOURCE) $(HEADER) tests/test_heap.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCE) tests/test_heap.c -o $@

demo: $(BUILD)/demo
	./$(BUILD)/demo

test: $(BUILD)/test_heap
	./$(BUILD)/test_heap

sanitize: | $(BUILD)
	$(CC) $(CPPFLAGS) -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined $(SOURCE) tests/test_heap.c -o $(BUILD)/test_heap_sanitized
	ASAN_OPTIONS="$(ASAN_OPTIONS)" ./$(BUILD)/test_heap_sanitized

# Optional GNU-linker allocation accounting and allocation-failure tests.
memory: | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCE) tests/allocation_checks.c -Wl,--wrap=malloc,--wrap=calloc,--wrap=free -o $(BUILD)/allocation_checks
	./$(BUILD)/allocation_checks

clean:
	rm -rf $(BUILD)
