#include "tinyos/platform.h"

#include "tinyos/display.h"
#include "tinyos/input.h"
#include "tinyos/port_io.h"

#define COM1_PORT 0x3F8u
#define SERIAL_READY_POLL_LIMIT 100000u

const struct input_backend *x86_64_keyboard_backend(void);
const struct display_backend *x86_64_text_display_backend(void);
static const struct tinyos_boot_info *g_boot_info = 0;

static void io_wait(void) {
    outb(0x80u, 0u);
}

static void x86_64_console_init(void) {
    outb(COM1_PORT + 1u, 0x00u);
    outb(COM1_PORT + 3u, 0x80u);
    outb(COM1_PORT + 0u, 0x03u);
    outb(COM1_PORT + 1u, 0x00u);
    outb(COM1_PORT + 3u, 0x03u);
    outb(COM1_PORT + 2u, 0xC7u);
    outb(COM1_PORT + 4u, 0x0Bu);
    io_wait();
}

static void x86_64_console_write_char(char ch) {
    uint32_t spins = 0u;

    while ((inb(COM1_PORT + 5u) & 0x20u) == 0u) {
        ++spins;
        if (spins >= SERIAL_READY_POLL_LIMIT) {
            break;
        }
    }

    outb(COM1_PORT, (uint8_t)ch);
}

static uint64_t x86_64_qemu_boot_heap_limit(void) {
    if ((g_boot_info != 0) && (g_boot_info->boot_method == TINYOS_BOOT_METHOD_UEFI)) {
        return 0x04000000u;
    }

    return 0x800000u;
}

static struct tinyos_platform_ops g_x86_64_qemu_platform = {
    .name = "x86_64-qemu",
    .early_init = 0,
    .boot_heap_limit = x86_64_qemu_boot_heap_limit,
    .console = {
        .init = x86_64_console_init,
        .write_char = x86_64_console_write_char
    },
    .display = 0,
    .input = 0
};

const struct tinyos_platform_ops *tinyos_platform_current(void) {
    return &g_x86_64_qemu_platform;
}

void platform_set_boot_info(const struct tinyos_boot_info *boot_info) {
    g_boot_info = boot_info;
}

void platform_init(void) {
    struct tinyos_platform_ops *platform = &g_x86_64_qemu_platform;

    if ((g_boot_info != 0) && (g_boot_info->boot_method == TINYOS_BOOT_METHOD_UEFI) &&
        g_boot_info->framebuffer.available && (g_boot_info->framebuffer.base != 0u)) {
        platform->display = 0;
    } else {
        platform->display = x86_64_text_display_backend();
    }
    platform->input = x86_64_keyboard_backend();

    if ((platform != 0) && (platform->early_init != 0)) {
        platform->early_init();
    }

    display_register_backend(platform->display);
    input_register_backend(platform->input);
}
