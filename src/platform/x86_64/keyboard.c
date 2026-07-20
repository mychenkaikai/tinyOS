#include "tinyos/input.h"
#include "tinyos/port_io.h"

#include <stdbool.h>
#include <stdint.h>

#define PS2_DATA_PORT 0x60u

static void x86_64_keyboard_init(void);
static void x86_64_keyboard_handle_irq(void);
static const struct input_backend g_x86_64_keyboard_backend = {
    .init = x86_64_keyboard_init,
    .handle_irq = x86_64_keyboard_handle_irq
};

static char translate_set1_scancode(uint8_t scancode) {
    static const char lookup[128] = {
        [0x02] = '1',
        [0x03] = '2',
        [0x04] = '3',
        [0x05] = '4',
        [0x06] = '5',
        [0x07] = '6',
        [0x08] = '7',
        [0x09] = '8',
        [0x0A] = '9',
        [0x0B] = '0',
        [0x0C] = '-',
        [0x0D] = '=',
        [0x0E] = '\b',
        [0x0F] = '\t',
        [0x10] = 'q',
        [0x11] = 'w',
        [0x12] = 'e',
        [0x13] = 'r',
        [0x14] = 't',
        [0x15] = 'y',
        [0x16] = 'u',
        [0x17] = 'i',
        [0x18] = 'o',
        [0x19] = 'p',
        [0x1A] = '[',
        [0x1B] = ']',
        [0x1C] = '\n',
        [0x1E] = 'a',
        [0x1F] = 's',
        [0x20] = 'd',
        [0x21] = 'f',
        [0x22] = 'g',
        [0x23] = 'h',
        [0x24] = 'j',
        [0x25] = 'k',
        [0x26] = 'l',
        [0x27] = ';',
        [0x28] = '\'',
        [0x29] = '`',
        [0x2B] = '\\',
        [0x2C] = 'z',
        [0x2D] = 'x',
        [0x2E] = 'c',
        [0x2F] = 'v',
        [0x30] = 'b',
        [0x31] = 'n',
        [0x32] = 'm',
        [0x33] = ',',
        [0x34] = '.',
        [0x35] = '/',
        [0x39] = ' '
    };

    if (scancode >= (uint8_t)sizeof(lookup)) {
        return '\0';
    }

    return lookup[scancode];
}

static void x86_64_keyboard_init(void) {
}

static void x86_64_keyboard_handle_irq(void) {
    uint8_t raw_scancode = inb(PS2_DATA_PORT);
    bool pressed = (raw_scancode & 0x80u) == 0u;
    uint8_t base_scancode = (uint8_t)(raw_scancode & 0x7Fu);
    struct input_event event;

    if ((raw_scancode == 0xE0u) || (raw_scancode == 0xE1u)) {
        return;
    }

    event.type = INPUT_EVENT_KEY;
    event.scancode = base_scancode;
    event.character = translate_set1_scancode(base_scancode);
    event.pressed = pressed;
    input_push_event(&event);
}

const struct input_backend *x86_64_keyboard_backend(void) {
    return &g_x86_64_keyboard_backend;
}
