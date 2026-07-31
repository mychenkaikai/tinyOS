#include "tinyos/platform.h"

#include <stdint.h>

#define AARCH64_VIRT_UART_BASE 0x09000000u
#define AARCH64_VIRT_UART_DR 0x00u
#define AARCH64_VIRT_UART_FR 0x18u
#define AARCH64_VIRT_UART_TXFF (1u << 5)
#define AARCH64_VIRT_UART_POLL_LIMIT 100000u

const struct tinyos_boot_info g_aarch64_boot_info = {
    .revision = TINYOS_BOOT_INFO_REVISION,
    .boot_method = TINYOS_BOOT_METHOD_UNKNOWN,
    .memory_map_address = 0u,
    .memory_map_size = 0u,
    .memory_map_descriptor_size = 0u,
    .rsdp_address = 0u,
    .framebuffer = {0u, 0u, 0u, 0u, 0u, false},
    .boot_path = "qemu-virt->kernel_main"
};

static volatile uint32_t *uart_reg(uint32_t offset) {
    return (volatile uint32_t *)(uintptr_t)(AARCH64_VIRT_UART_BASE + offset);
}

static void aarch64_console_init(void) {
}

static void aarch64_console_write_char(char ch) {
    uint32_t spins = 0u;

    while (((*uart_reg(AARCH64_VIRT_UART_FR)) & AARCH64_VIRT_UART_TXFF) != 0u) {
        ++spins;
        if (spins >= AARCH64_VIRT_UART_POLL_LIMIT) {
            break;
        }
    }

    *uart_reg(AARCH64_VIRT_UART_DR) = (uint32_t)(uint8_t)ch;
}

static uint64_t aarch64_virt_boot_heap_limit(void) {
    return 0x48000000u;
}

static struct tinyos_platform_ops g_aarch64_virt_platform = {
    .name = "aarch64-virt",
    .early_init = 0,
    .boot_heap_limit = aarch64_virt_boot_heap_limit,
    .console = {
        .init = aarch64_console_init,
        .write_char = aarch64_console_write_char
    },
    .display = 0,
    .input = 0
};

const struct tinyos_platform_ops *tinyos_platform_current(void) {
    return &g_aarch64_virt_platform;
}

void platform_set_boot_info(const struct tinyos_boot_info *boot_info) {
    (void)boot_info;
}

void platform_init(void) {
    if (g_aarch64_virt_platform.early_init != 0) {
        g_aarch64_virt_platform.early_init();
    }
}
