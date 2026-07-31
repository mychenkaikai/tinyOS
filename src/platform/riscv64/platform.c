#include "tinyos/platform.h"

#include <stdint.h>

#define RISCV64_VIRT_UART_BASE 0x10000000u
#define RISCV64_VIRT_UART_THR 0x00u
#define RISCV64_VIRT_UART_LSR 0x05u
#define RISCV64_VIRT_UART_LSR_THRE 0x20u
#define RISCV64_VIRT_UART_POLL_LIMIT 100000u

const struct tinyos_boot_info g_riscv64_boot_info = {
    .revision = TINYOS_BOOT_INFO_REVISION,
    .boot_method = TINYOS_BOOT_METHOD_UNKNOWN,
    .memory_map_address = 0u,
    .memory_map_size = 0u,
    .memory_map_descriptor_size = 0u,
    .rsdp_address = 0u,
    .framebuffer = {0u, 0u, 0u, 0u, 0u, false},
    .boot_path = "qemu-virt->kernel_main"
};

static volatile uint8_t *uart_reg(uint32_t offset) {
    return (volatile uint8_t *)(uintptr_t)(RISCV64_VIRT_UART_BASE + offset);
}

static void riscv64_console_init(void) {
}

static void riscv64_console_write_char(char ch) {
    uint32_t spins = 0u;

    while (((*uart_reg(RISCV64_VIRT_UART_LSR)) & RISCV64_VIRT_UART_LSR_THRE) == 0u) {
        ++spins;
        if (spins >= RISCV64_VIRT_UART_POLL_LIMIT) {
            break;
        }
    }

    *uart_reg(RISCV64_VIRT_UART_THR) = (uint8_t)ch;
}

static uint64_t riscv64_virt_boot_heap_limit(void) {
    return 0x88000000u;
}

static struct tinyos_platform_ops g_riscv64_virt_platform = {
    .name = "riscv64-virt",
    .early_init = 0,
    .boot_heap_limit = riscv64_virt_boot_heap_limit,
    .console = {
        .init = riscv64_console_init,
        .write_char = riscv64_console_write_char
    },
    .display = 0,
    .input = 0
};

const struct tinyos_platform_ops *tinyos_platform_current(void) {
    return &g_riscv64_virt_platform;
}

void platform_set_boot_info(const struct tinyos_boot_info *boot_info) {
    (void)boot_info;
}

void platform_init(void) {
    if (g_riscv64_virt_platform.early_init != 0) {
        g_riscv64_virt_platform.early_init();
    }
}
