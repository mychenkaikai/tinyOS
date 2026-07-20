#ifndef TINYOS_PLATFORM_H
#define TINYOS_PLATFORM_H

#include <stdint.h>

#include "tinyos/display.h"
#include "tinyos/input.h"

struct tinyos_console_ops {
    void (*init)(void);
    void (*write_char)(char ch);
};

struct tinyos_platform_ops {
    const char *name;
    void (*early_init)(void);
    uint64_t (*boot_heap_limit)(void);
    struct tinyos_console_ops console;
    const struct display_backend *display;
    const struct input_backend *input;
};

const struct tinyos_platform_ops *tinyos_platform_current(void);
void platform_init(void);

#endif
