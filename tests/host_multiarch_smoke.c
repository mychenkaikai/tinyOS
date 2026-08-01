#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "tinyos/arch.h"
#include "tinyos/boot_info.h"
#include "tinyos/display.h"
#include "tinyos/event_loop.h"
#include "tinyos/input.h"
#include "tinyos/platform.h"

#define HOST_EVENT_TASK_CAPACITY 8u
#define HOST_EVENT_LOOP_MAX_TICKS 240u
#define HOST_DISPLAY_WIDTH 80u
#define HOST_DISPLAY_HEIGHT 25u

extern void kernel_main(const struct tinyos_boot_info *boot_info);

uint8_t __kernel_end = 0u;

static const char *g_selected_arch = "aarch64";
static const char *g_selected_platform = "aarch64-host-smoke";
static const char *g_selected_profile = "headless";
static uint64_t g_host_ticks = 0u;
static bool g_interrupts_ready = false;
static bool g_interrupts_enabled = false;
static struct event_task *g_tasks[HOST_EVENT_TASK_CAPACITY];
static uint32_t g_task_count = 0u;
static uint32_t g_injected_events = 0u;
static char g_display_buffer[HOST_DISPLAY_HEIGHT][HOST_DISPLAY_WIDTH];
static uint32_t g_display_cursor_row = 0u;
static uint32_t g_display_cursor_col = 0u;

static void host_console_init(void) {
}

static void host_console_write_char(char ch) {
    putchar((int)(unsigned char)ch);
    fflush(stdout);
}

static void host_display_reset(void) {
    uint32_t row;
    uint32_t col;

    for (row = 0u; row < HOST_DISPLAY_HEIGHT; ++row) {
        for (col = 0u; col < HOST_DISPLAY_WIDTH; ++col) {
            g_display_buffer[row][col] = ' ';
        }
    }

    g_display_cursor_row = 0u;
    g_display_cursor_col = 0u;
}

static void host_display_init(void) {
    host_display_reset();
}

static void host_display_clear(void) {
    host_display_reset();
}

static void host_display_write_char(char ch) {
    if (ch == '\n') {
        if (g_display_cursor_row + 1u < HOST_DISPLAY_HEIGHT) {
            ++g_display_cursor_row;
        }
        g_display_cursor_col = 0u;
        return;
    }

    if (g_display_cursor_col >= HOST_DISPLAY_WIDTH) {
        if (g_display_cursor_row + 1u < HOST_DISPLAY_HEIGHT) {
            ++g_display_cursor_row;
        }
        g_display_cursor_col = 0u;
    }

    if ((g_display_cursor_row < HOST_DISPLAY_HEIGHT) && (g_display_cursor_col < HOST_DISPLAY_WIDTH)) {
        g_display_buffer[g_display_cursor_row][g_display_cursor_col] = ch;
        ++g_display_cursor_col;
    }
}

static bool host_display_dimensions(uint32_t *width, uint32_t *height) {
    if ((width == NULL) || (height == NULL)) {
        return false;
    }

    *width = HOST_DISPLAY_WIDTH;
    *height = HOST_DISPLAY_HEIGHT;
    return true;
}

static bool host_display_write_at(uint32_t row, uint32_t col, char ch) {
    if ((row >= HOST_DISPLAY_HEIGHT) || (col >= HOST_DISPLAY_WIDTH)) {
        return false;
    }

    g_display_buffer[row][col] = ch;
    return true;
}

static const struct display_backend g_host_display_backend = {
    .init = host_display_init,
    .clear = host_display_clear,
    .write_char = host_display_write_char,
    .dimensions = host_display_dimensions,
    .write_at = host_display_write_at
};

static void host_input_init(void) {
}

static void host_input_handle_irq(void) {
}

static const struct input_backend g_host_input_backend = {
    .init = host_input_init,
    .handle_irq = host_input_handle_irq
};

static const struct tinyos_console_ops g_host_console_ops = {
    .init = host_console_init,
    .write_char = host_console_write_char
};

static struct tinyos_platform_ops g_host_platform = {
    .name = "aarch64-host-smoke",
    .early_init = 0,
    .boot_heap_limit = 0,
    .console = {
        .init = host_console_init,
        .write_char = host_console_write_char
    },
    .display = 0,
    .input = &g_host_input_backend
};

static uint64_t host_boot_heap_limit(void) {
    return (uint64_t)(uintptr_t)&__kernel_end + 0x20000u;
}

static void host_arch_early_init(void) {
}

static void host_interrupts_init(uint32_t tick_hz) {
    (void)tick_hz;
    g_host_ticks = 0u;
    g_interrupts_ready = true;
    g_interrupts_enabled = false;
}

static void host_interrupts_enable(void) {
    g_interrupts_enabled = true;
}

static void host_interrupts_disable(void) {
    g_interrupts_enabled = false;
}

static uint64_t host_timer_ticks(void) {
    return g_host_ticks;
}

static bool host_interrupts_ready(void) {
    return g_interrupts_ready;
}

static void host_cpu_idle(void) {
    ++g_host_ticks;
}

static struct tinyos_arch_ops g_host_arch = {
    .name = "aarch64",
    .early_init = host_arch_early_init,
    .interrupts_init = host_interrupts_init,
    .interrupts_enable = host_interrupts_enable,
    .interrupts_disable = host_interrupts_disable,
    .timer_ticks = host_timer_ticks,
    .interrupts_ready = host_interrupts_ready,
    .cpu_idle = host_cpu_idle
};

static void configure_mode(const char *mode) {
    if ((mode != NULL) && (strcmp(mode, "riscv64") == 0)) {
        g_selected_arch = "riscv64";
        g_selected_platform = "riscv64-host-smoke";
    } else {
        g_selected_arch = "aarch64";
        g_selected_platform = "aarch64-host-smoke";
    }

    g_host_arch.name = g_selected_arch;
    g_host_platform.name = g_selected_platform;
    g_host_platform.boot_heap_limit = host_boot_heap_limit;
    g_host_platform.console = g_host_console_ops;
    g_host_platform.input = &g_host_input_backend;
}

static void configure_profile(const char *profile) {
    if ((profile != NULL) && (strcmp(profile, "gui") == 0)) {
        g_selected_profile = "gui";
        g_host_platform.display = &g_host_display_backend;
    } else {
        g_selected_profile = "headless";
        g_host_platform.display = (const struct display_backend *)0;
    }
}

const struct tinyos_arch_ops *tinyos_arch_current(void) {
    return &g_host_arch;
}

const struct tinyos_platform_ops *tinyos_platform_current(void) {
    return &g_host_platform;
}

void platform_set_boot_info(const struct tinyos_boot_info *boot_info) {
    (void)boot_info;
}

void platform_init(void) {
    if (g_host_platform.display != (const struct display_backend *)0) {
        display_register_backend(g_host_platform.display);
    }
    input_register_backend(&g_host_input_backend);
}

void event_loop_init(void) {
    uint32_t index;

    for (index = 0u; index < HOST_EVENT_TASK_CAPACITY; ++index) {
        g_tasks[index] = (struct event_task *)0;
    }

    g_task_count = 0u;
}

bool event_loop_add_periodic(struct event_task *task, uint64_t period_ticks, event_task_fn handler, void *context) {
    if ((task == NULL) || (handler == NULL) || (period_ticks == 0u) || (g_task_count >= HOST_EVENT_TASK_CAPACITY)) {
        return false;
    }

    task->period_ticks = period_ticks;
    task->next_run_tick = g_host_ticks + period_ticks;
    task->handler = handler;
    task->context = context;
    g_tasks[g_task_count] = task;
    ++g_task_count;
    return true;
}

static void inject_input_events_if_needed(void) {
    struct input_event event;

    if (g_injected_events != 0u) {
        return;
    }

    event.type = INPUT_EVENT_KEY;
    event.pressed = true;

    if (strcmp(g_selected_profile, "gui") == 0) {
        event.scancode = 0x04u;
        event.character = '3';
        (void)input_push_event(&event);

        event.scancode = 0x17u;
        event.character = 'i';
        (void)input_push_event(&event);

        event.scancode = 0x03u;
        event.character = '2';
        (void)input_push_event(&event);

        event.scancode = 0x12u;
        event.character = 'e';
        (void)input_push_event(&event);

        event.scancode = 0x2Du;
        event.character = 'x';
        (void)input_push_event(&event);

        event.scancode = 0x0Bu;
        event.character = '0';
        (void)input_push_event(&event);

        g_injected_events = 6u;
        return;
    }

    event.scancode = 0x0Fu;
    event.character = '\t';
    (void)input_push_event(&event);

    event.scancode = 0x1Cu;
    event.character = '\n';
    (void)input_push_event(&event);

    event.scancode = 0x2Du;
    event.character = 'x';
    (void)input_push_event(&event);

    g_injected_events = 3u;
}

static void dump_display_snapshot(void) {
    uint32_t row;

    if (strcmp(g_selected_profile, "gui") != 0) {
        return;
    }

    for (row = 0u; row < HOST_DISPLAY_HEIGHT; ++row) {
        int32_t last_non_space = (int32_t)HOST_DISPLAY_WIDTH - 1;

        while ((last_non_space >= 0) && (g_display_buffer[row][(uint32_t)last_non_space] == ' ')) {
            --last_non_space;
        }

        printf("[display] row=%02u text=", row);
        if (last_non_space < 0) {
            printf("\n");
            continue;
        }

        fwrite(g_display_buffer[row], 1u, (size_t)last_non_space + 1u, stdout);
        printf("\n");
    }
}

void event_loop_run(void) {
    while (g_host_ticks < HOST_EVENT_LOOP_MAX_TICKS) {
        uint32_t index;

        if (g_interrupts_enabled && (g_host_ticks == 0u)) {
            inject_input_events_if_needed();
        }

        for (index = 0u; index < g_task_count; ++index) {
            struct event_task *task = g_tasks[index];

            if ((task != NULL) && (g_host_ticks >= task->next_run_tick)) {
                task->next_run_tick += task->period_ticks;
                task->handler(task->context);
            }
        }

        ++g_host_ticks;
    }

    dump_display_snapshot();
    printf("[host-smoke] completed arch=%s profile=%s ticks=%llu events=%u\n",
           g_selected_arch,
           g_selected_profile,
           (unsigned long long)g_host_ticks,
           g_injected_events);
    exit(0);
}

int main(int argc, char **argv) {
    static const struct tinyos_boot_info boot_info = {
        .revision = TINYOS_BOOT_INFO_REVISION,
        .boot_method = TINYOS_BOOT_METHOD_UNKNOWN,
        .memory_map_address = 0u,
        .memory_map_size = 0u,
        .memory_map_descriptor_size = 0u,
        .rsdp_address = 0u,
        .framebuffer = {0u, 0u, 0u, 0u, 0u, false},
        .boot_path = "host-smoke->kernel_main"
    };

    if (argc > 1) {
        configure_mode(argv[1]);
    } else {
        configure_mode("aarch64");
    }

    if (argc > 2) {
        configure_profile(argv[2]);
    } else {
        configure_profile("headless");
    }

    kernel_main(&boot_info);
    return 0;
}
