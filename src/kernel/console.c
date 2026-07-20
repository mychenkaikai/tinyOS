#include "tinyos/console.h"
#include "tinyos/display.h"
#include "tinyos/port_io.h"

#define COM1_PORT 0x3F8u

static bool g_console_display_mirror = true;

static void io_wait(void) {
    outb(0x80u, 0u);
}

static void serial_init(void) {
    outb(COM1_PORT + 1u, 0x00u);
    outb(COM1_PORT + 3u, 0x80u);
    outb(COM1_PORT + 0u, 0x03u);
    outb(COM1_PORT + 1u, 0x00u);
    outb(COM1_PORT + 3u, 0x03u);
    outb(COM1_PORT + 2u, 0xC7u);
    outb(COM1_PORT + 4u, 0x0Bu);
    io_wait();
}

static void serial_write_char(char ch) {
    while ((inb(COM1_PORT + 5u) & 0x20u) == 0u) {
    }
    outb(COM1_PORT, (uint8_t)ch);
}

void console_init(void) {
    serial_init();
}

void console_set_display_mirror(bool enabled) {
    g_console_display_mirror = enabled;
}

void console_write_char(char ch) {
    if (ch == '\n') {
        serial_write_char('\r');
    }

    serial_write_char(ch);
    if (g_console_display_mirror) {
        display_write_char(ch);
    }
}

void console_write(const char *message) {
    while (*message != '\0') {
        console_write_char(*message);
        ++message;
    }
}

void console_write_line(const char *message) {
    console_write(message);
    console_write_char('\n');
}

void console_write_hex64(uint64_t value) {
    static const char digits[] = "0123456789ABCDEF";
    int shift;

    console_write("0x");
    for (shift = 60; shift >= 0; shift -= 4) {
        console_write_char(digits[(value >> (uint32_t)shift) & 0x0Fu]);
    }
}

void console_write_u64(uint64_t value) {
    char buffer[20];
    uint32_t count = 0;

    if (value == 0u) {
        console_write_char('0');
        return;
    }

    while (value != 0u) {
        buffer[count] = (char)('0' + (value % 10u));
        value /= 10u;
        ++count;
    }

    while (count > 0u) {
        --count;
        console_write_char(buffer[count]);
    }
}
