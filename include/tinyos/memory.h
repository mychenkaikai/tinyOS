#ifndef TINYOS_MEMORY_H
#define TINYOS_MEMORY_H

#include <stddef.h>
#include <stdint.h>

void memory_init(uintptr_t heap_limit);
void *memory_alloc(size_t size, size_t alignment);
uintptr_t memory_heap_start(void);
uintptr_t memory_heap_end(void);
uintptr_t memory_bytes_used(void);

#endif
