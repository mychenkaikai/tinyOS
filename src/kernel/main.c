#include <stddef.h>
#include <stdint.h>

#include "tinyos/arch.h"
#include "tinyos/boot_info.h"
#include "tinyos/console.h"
#include "tinyos/display.h"
#include "tinyos/event_loop.h"
#include "tinyos/gui.h"
#include "tinyos/input.h"
#include "tinyos/memory.h"
#include "tinyos/platform.h"
#include "tinyos/uefi_boot_demo.h"

#define HEARTBEAT_PERIOD_TICKS 100u
#define INPUT_POLL_PERIOD_TICKS 2u
#define GUI_RENDER_PERIOD_TICKS 5u

struct heartbeat_context {
    uint64_t runs;
};

struct input_context {
    uint64_t events_seen;
};

static void gui_task(void *context) {
    (void)context;
    gui_render();
}

static const struct tinyos_arch_ops *current_arch(void) {
    return tinyos_arch_current();
}

static uint64_t current_ticks(void) {
    const struct tinyos_arch_ops *arch = current_arch();

    if ((arch == NULL) || (arch->timer_ticks == NULL)) {
        return 0u;
    }

    return arch->timer_ticks();
}

static void halt_forever(void) {
    const struct tinyos_arch_ops *arch = current_arch();

    if ((arch != NULL) && (arch->interrupts_disable != NULL)) {
        arch->interrupts_disable();
    }

    for (;;) {
        if ((arch != NULL) && (arch->cpu_idle != NULL)) {
            arch->cpu_idle();
        }
    }
}

// #region debug-point B:kernel-main-entry
static void debugcon_write(char marker) {
#if defined(__x86_64__)
    __asm__ volatile ("outb %0, %1" : : "a"(marker), "Nd"((uint16_t)0x402));
#else
    (void)marker;
#endif
}
// #endregion

static bool is_printable_ascii(char ch) {
    return (ch >= 32) && (ch <= 126);
}

static void *must_alloc(size_t size, size_t alignment, const char *name) {
    void *memory = memory_alloc(size, alignment);
    if (memory == (void *)0) {
        console_write("Allocation failed for ");
        console_write_line(name);
        halt_forever();
    }
    return memory;
}

static void heartbeat_task(void *context) {
    struct heartbeat_context *state = (struct heartbeat_context *)context;

    ++state->runs;
    gui_note_heartbeat();
    console_write("[event] heartbeat=");
    console_write_u64(state->runs);
    console_write(" ticks=");
    console_write_u64(current_ticks());
    console_write(" heap_used=");
    console_write_u64(memory_bytes_used());
    console_write_char('\n');
}

static void write_key_label(char ch) {
    if (ch == '\n') {
        console_write("enter");
        return;
    }

    if (ch == '\t') {
        console_write("tab");
        return;
    }

    if (ch == '\b') {
        console_write("backspace");
        return;
    }

    if (ch == ' ') {
        console_write("space");
        return;
    }

    if (is_printable_ascii(ch)) {
        console_write_char(ch);
        return;
    }

    console_write("none");
}

static const char *boot_method_name(uint32_t boot_method) {
    if (boot_method == TINYOS_BOOT_METHOD_UEFI) {
        return "uefi";
    }

    if (boot_method == TINYOS_BOOT_METHOD_BIOS) {
        return "bios";
    }

    return "unknown";
}

static bool using_boot_framebuffer(const struct tinyos_boot_info *boot_info) {
    return (boot_info != (const struct tinyos_boot_info *)0) &&
           (boot_info->boot_method == TINYOS_BOOT_METHOD_UEFI) &&
           boot_info->framebuffer.available &&
           (boot_info->framebuffer.base != 0u);
}

static const char *display_mode_name(const struct tinyos_boot_info *boot_info, const struct tinyos_platform_ops *platform) {
    if (using_boot_framebuffer(boot_info)) {
        return "boot framebuffer -> GUI renderer";
    }

    if ((platform != (const struct tinyos_platform_ops *)0) && (platform->display != (const struct display_backend *)0)) {
        return "platform display backend attached";
    }

    return "no display backend attached";
}

static const char *input_mode_name(const struct tinyos_boot_info *boot_info, const struct tinyos_platform_ops *platform) {
    if (using_boot_framebuffer(boot_info) && (platform != (const struct tinyos_platform_ops *)0) &&
        (platform->input != (const struct input_backend *)0)) {
        return "platform input backend -> GUI path";
    }

    if ((platform != (const struct tinyos_platform_ops *)0) && (platform->input != (const struct input_backend *)0)) {
        return "platform input backend attached";
    }

    return "no input backend attached";
}

static void input_task(void *context) {
    struct input_context *state = (struct input_context *)context;
    struct input_event event;

    while (input_pop_event(&event)) {
        if (event.type != INPUT_EVENT_KEY) {
            continue;
        }

        gui_handle_input_event(&event);
        if (!event.pressed) {
            continue;
        }

        ++state->events_seen;
        console_write("[input] key#=");
        console_write_u64(state->events_seen);
        console_write(" scancode=");
        console_write_hex64((uint64_t)event.scancode);
        console_write(" char=");
        write_key_label(event.character);
        console_write_char('\n');
    }
}

void kernel_main(const struct tinyos_boot_info *boot_info) {
    struct event_task *heartbeat;
    struct heartbeat_context *heartbeat_state;
    struct event_task *input_poller;
    struct input_context *input_state;
    struct event_task *gui_renderer;
    const struct tinyos_arch_ops *arch = current_arch();
    const struct tinyos_platform_ops *platform = tinyos_platform_current();

    debugcon_write('K');
    if (boot_info != (void *)0) {
        if (boot_info->boot_method == TINYOS_BOOT_METHOD_UEFI) {
            debugcon_write('U');
            debugcon_write('S');
            (void)tinyos_uefi_boot_demo_run(boot_info);
        }
    }

    if ((arch != (void *)0) && (arch->early_init != (void *)0)) {
        arch->early_init();
    }

    platform_set_boot_info(boot_info);
    platform_init();
    console_init();
    if ((boot_info != (void *)0) && (boot_info->boot_method == TINYOS_BOOT_METHOD_UEFI)) {
        console_set_display_mirror(false);
    }
    display_init();
    input_init();
    console_write_line("tinyOS kernel bootstrap ready.");
    console_write("Architecture: ");
    console_write_line((arch != (const struct tinyos_arch_ops *)0) ? arch->name : "unknown");
    if (using_boot_framebuffer(boot_info)) {
        console_write_line("Phase: UEFI framebuffer GUI runtime ready.");
    } else {
        console_write_line("Phase: generic kernel runtime ready.");
    }
    console_write("Boot method: ");
    console_write_line((boot_info != (void *)0) ? boot_method_name(boot_info->boot_method) : "unknown");
    console_write("Boot path: ");
    console_write_line(((boot_info != (void *)0) && (boot_info->boot_path[0] != '\0')) ? boot_info->boot_path : "unknown");
    console_write("Platform: ");
    console_write_line((platform != (void *)0) ? platform->name : "unknown");
    console_write("Display: ");
    console_write_line(display_mode_name(boot_info, platform));
    console_write("Input: ");
    console_write_line(input_mode_name(boot_info, platform));

    memory_init(((platform != (void *)0) && (platform->boot_heap_limit != (void *)0)) ? platform->boot_heap_limit() : 0x200000u);
    console_write("Early heap: start=");
    console_write_hex64(memory_heap_start());
    console_write(" end=");
    console_write_hex64(memory_heap_end());
    console_write_char('\n');

    event_loop_init();
    if ((arch != (void *)0) && (arch->interrupts_init != (void *)0)) {
        arch->interrupts_init(100u);
    }
    console_write("Interrupts: arch backend ready=");
    console_write_line(((arch != (void *)0) && (arch->interrupts_ready != (void *)0) && arch->interrupts_ready()) ? "yes" : "no");

    heartbeat = (struct event_task *)must_alloc(sizeof(*heartbeat), 16u, "heartbeat task");
    heartbeat_state = (struct heartbeat_context *)must_alloc(sizeof(*heartbeat_state), 16u, "heartbeat context");
    heartbeat_state->runs = 0u;

    if (!event_loop_add_periodic(heartbeat, HEARTBEAT_PERIOD_TICKS, heartbeat_task, heartbeat_state)) {
        console_write_line("Failed to register heartbeat task.");
        halt_forever();
    }

    input_poller = (struct event_task *)must_alloc(sizeof(*input_poller), 16u, "input task");
    input_state = (struct input_context *)must_alloc(sizeof(*input_state), 16u, "input context");
    input_state->events_seen = 0u;

    if (!event_loop_add_periodic(input_poller, INPUT_POLL_PERIOD_TICKS, input_task, input_state)) {
        console_write_line("Failed to register input task.");
        halt_forever();
    }

    gui_renderer = (struct event_task *)must_alloc(sizeof(*gui_renderer), 16u, "gui task");
    gui_init(boot_info);
    console_set_display_mirror(false);
    gui_render();

    if (!event_loop_add_periodic(gui_renderer, GUI_RENDER_PERIOD_TICKS, gui_task, (void *)0)) {
        console_write_line("Failed to register gui task.");
        halt_forever();
    }

    if (using_boot_framebuffer(boot_info)) {
        console_write_line("[gui] Boot framebuffer GUI active. Screen owned by GUI, logs continue on serial.");
    } else if ((platform != (const struct tinyos_platform_ops *)0) && (platform->display != (const struct display_backend *)0)) {
        console_write_line("[gui] Platform display GUI active. Screen owned by GUI, logs continue on serial.");
    } else {
        console_write_line("[gui] Headless GUI loop active. Logs continue on serial.");
    }
    console_write_line("Event loop: heartbeat, input and gui tasks armed, enabling interrupts.");
    if ((arch != (void *)0) && (arch->interrupts_enable != (void *)0)) {
        arch->interrupts_enable();
    }
    event_loop_run();
}
