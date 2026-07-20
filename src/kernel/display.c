#include "tinyos/display.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const struct display_backend *g_display_backend = NULL;

void display_register_backend(const struct display_backend *backend) {
    g_display_backend = backend;
}

void display_init(void) {
    if ((g_display_backend != NULL) && (g_display_backend->init != NULL)) {
        g_display_backend->init();
    }
}

void display_clear(void) {
    if ((g_display_backend != NULL) && (g_display_backend->clear != NULL)) {
        g_display_backend->clear();
    }
}

void display_write_char(char ch) {
    if ((g_display_backend != NULL) && (g_display_backend->write_char != NULL)) {
        g_display_backend->write_char(ch);
    }
}

void display_write(const char *message) {
    while ((message != NULL) && (*message != '\0')) {
        display_write_char(*message);
        ++message;
    }
}

void display_write_line(const char *message) {
    display_write(message);
    display_write_char('\n');
}

bool display_dimensions(uint32_t *width, uint32_t *height) {
    if ((g_display_backend == NULL) || (g_display_backend->dimensions == NULL)) {
        return false;
    }

    return g_display_backend->dimensions(width, height);
}

bool display_write_at(uint32_t row, uint32_t col, char ch) {
    if ((g_display_backend == NULL) || (g_display_backend->write_at == NULL)) {
        return false;
    }

    return g_display_backend->write_at(row, col, ch);
}
