#ifndef TINYOS_DISPLAY_H
#define TINYOS_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

struct display_backend {
    void (*init)(void);
    void (*clear)(void);
    void (*write_char)(char ch);
    bool (*dimensions)(uint32_t *width, uint32_t *height);
    bool (*write_at)(uint32_t row, uint32_t col, char ch);
};

void display_register_backend(const struct display_backend *backend);
void display_init(void);
void display_clear(void);
void display_write_char(char ch);
void display_write(const char *message);
void display_write_line(const char *message);
bool display_dimensions(uint32_t *width, uint32_t *height);
bool display_write_at(uint32_t row, uint32_t col, char ch);

#endif
