#include "tinyos/console.h"
#include "tinyos/display.h"
#include "tinyos/platform.h"

static bool g_console_display_mirror = true;
static const struct tinyos_console_ops *current_console(void) {
    const struct tinyos_platform_ops *platform = tinyos_platform_current();

    if (platform == (const struct tinyos_platform_ops *)0) {
        return (const struct tinyos_console_ops *)0;
    }

    return &platform->console;
}

void console_init(void) {
    const struct tinyos_console_ops *console = current_console();

    if ((console != (const struct tinyos_console_ops *)0) && (console->init != (void *)0)) {
        console->init();
    }
}

void console_set_display_mirror(bool enabled) {
    g_console_display_mirror = enabled;
}

void console_write_char(char ch) {
    const struct tinyos_console_ops *console = current_console();

    if (ch == '\n') {
        if ((console != (const struct tinyos_console_ops *)0) && (console->write_char != (void *)0)) {
            console->write_char('\r');
        }
    }

    if ((console != (const struct tinyos_console_ops *)0) && (console->write_char != (void *)0)) {
        console->write_char(ch);
    }

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
