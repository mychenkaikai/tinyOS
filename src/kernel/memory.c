#include "tinyos/memory.h"

extern uint8_t __kernel_end;

static uintptr_t g_heap_start = 0;
static uintptr_t g_heap_current = 0;
static uintptr_t g_heap_limit = 0;

static uintptr_t align_up(uintptr_t value, size_t alignment) {
    uintptr_t mask;

    if (alignment == 0u) {
        return value;
    }

    mask = (uintptr_t)alignment - 1u;
    return (value + mask) & ~mask;
}

void memory_init(uintptr_t heap_limit) {
    g_heap_start = align_up((uintptr_t)&__kernel_end, 16u);
    g_heap_current = g_heap_start;
    g_heap_limit = heap_limit;
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
