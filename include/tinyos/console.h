#ifndef TINYOS_CONSOLE_H
#define TINYOS_CONSOLE_H

#include <stdbool.h>
#include <stdint.h>

void console_init(void);
void console_set_display_mirror(bool enabled);
void console_write_char(char ch);
void console_write(const char *message);
void console_write_line(const char *message);
void console_write_hex64(uint64_t value);
void console_write_u64(uint64_t value);

#endif
