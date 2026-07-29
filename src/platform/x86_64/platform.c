#include "tinyos/platform.h"

#include "tinyos/display.h"
#include "tinyos/input.h"

const struct input_backend *x86_64_keyboard_backend(void);
const struct display_backend *x86_64_text_display_backend(void);
static const struct tinyos_boot_info *g_boot_info = 0;

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
    .console = {0, 0},
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
