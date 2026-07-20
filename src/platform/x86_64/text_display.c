#include "tinyos/display.h"

#include <stdbool.h>
#include <stdint.h>

#define VGA_WIDTH 80u
#define VGA_HEIGHT 25u
#define VGA_ATTR 0x0Fu

static volatile uint16_t *const g_vga_buffer = (uint16_t *)0xB8000;
static uint32_t g_cursor_row = 0;
static uint32_t g_cursor_col = 0;
static bool x86_64_text_display_dimensions(uint32_t *width, uint32_t *height);
static bool x86_64_text_display_write_at(uint32_t row, uint32_t col, char ch);
static void x86_64_text_display_clear(void);
static void x86_64_text_display_write_char(char ch);
void x86_64_text_display_init(void);
static const struct display_backend g_x86_64_text_display_backend = {
    .init = x86_64_text_display_init,
    .clear = x86_64_text_display_clear,
    .write_char = x86_64_text_display_write_char,
    .dimensions = x86_64_text_display_dimensions,
    .write_at = x86_64_text_display_write_at
};

static bool x86_64_text_display_dimensions(uint32_t *width, uint32_t *height) {
    if ((width == 0) || (height == 0)) {
        return false;
    }

    *width = VGA_WIDTH;
    *height = VGA_HEIGHT;
    return true;
}

static bool x86_64_text_display_write_at(uint32_t row, uint32_t col, char ch) {
    if ((row >= VGA_HEIGHT) || (col >= VGA_WIDTH)) {
        return false;
    }

    g_vga_buffer[row * VGA_WIDTH + col] = ((uint16_t)VGA_ATTR << 8) | (uint16_t)ch;
    return true;
}

static void x86_64_text_display_clear(void) {
    uint32_t index;

    for (index = 0; index < VGA_WIDTH * VGA_HEIGHT; ++index) {
        g_vga_buffer[index] = ((uint16_t)VGA_ATTR << 8) | (uint16_t)' ';
    }

    g_cursor_row = 0;
    g_cursor_col = 0;
}

static void x86_64_text_display_scroll_if_needed(void) {
    uint32_t row;
    uint32_t col;

    if (g_cursor_row < VGA_HEIGHT) {
        return;
    }

    for (row = 1; row < VGA_HEIGHT; ++row) {
        for (col = 0; col < VGA_WIDTH; ++col) {
            g_vga_buffer[(row - 1u) * VGA_WIDTH + col] = g_vga_buffer[row * VGA_WIDTH + col];
        }
    }

    for (col = 0; col < VGA_WIDTH; ++col) {
        g_vga_buffer[(VGA_HEIGHT - 1u) * VGA_WIDTH + col] = ((uint16_t)VGA_ATTR << 8) | (uint16_t)' ';
    }

    g_cursor_row = VGA_HEIGHT - 1u;
}

static void x86_64_text_display_write_char(char ch) {
    if (ch == '\n') {
        g_cursor_col = 0;
        ++g_cursor_row;
        x86_64_text_display_scroll_if_needed();
        return;
    }

    (void)x86_64_text_display_write_at(g_cursor_row, g_cursor_col, ch);
    ++g_cursor_col;

    if (g_cursor_col >= VGA_WIDTH) {
        g_cursor_col = 0;
        ++g_cursor_row;
        x86_64_text_display_scroll_if_needed();
    }
}

void x86_64_text_display_init(void) {
    x86_64_text_display_clear();
}

const struct display_backend *x86_64_text_display_backend(void) {
    return &g_x86_64_text_display_backend;
}
