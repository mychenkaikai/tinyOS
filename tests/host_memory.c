#include "tinyos/memory.h"

#include <stdint.h>

#define HOST_HEAP_CAPACITY 0x20000u

static uint8_t g_host_heap[HOST_HEAP_CAPACITY];
static uintptr_t g_heap_start = 0u;
static uintptr_t g_heap_current = 0u;
static uintptr_t g_heap_limit = 0u;

static uintptr_t align_up(uintptr_t value, size_t alignment) {
    uintptr_t mask;

    if (alignment == 0u) {
        return value;
    }

    mask = (uintptr_t)alignment - 1u;
    return (value + mask) & ~mask;
}

void memory_init(uintptr_t heap_limit) {
    g_heap_start = align_up((uintptr_t)&g_host_heap[0], 16u);
    g_heap_current = g_heap_start;
    g_heap_limit = heap_limit;

    if (g_heap_limit > (uintptr_t)(&g_host_heap[HOST_HEAP_CAPACITY])) {
        g_heap_limit = (uintptr_t)(&g_host_heap[HOST_HEAP_CAPACITY]);
    }

    if (g_heap_limit < g_heap_start) {
        g_heap_limit = g_heap_start;
    }
}

void *memory_alloc(size_t size, size_t alignment) {
    uintptr_t base;
    uintptr_t next;

    if (alignment == 0u) {
        alignment = 16u;
    }

    base = align_up(g_heap_current, alignment);
    next = base + (uintptr_t)size;
    if ((next < base) || (next > g_heap_limit)) {
        return (void *)0;
    }

    g_heap_current = next;
    return (void *)base;
}

uintptr_t memory_heap_start(void) {
    return g_heap_start;
}

uintptr_t memory_heap_end(void) {
    return g_heap_limit;
}

uintptr_t memory_bytes_used(void) {
    return g_heap_current - g_heap_start;
}
