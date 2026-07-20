#include "tinyos/input.h"

#include <stddef.h>

#define INPUT_QUEUE_CAPACITY 32u

static const struct input_backend *g_input_backend = NULL;
static struct input_event g_input_queue[INPUT_QUEUE_CAPACITY];
static uint32_t g_input_read_index = 0;
static uint32_t g_input_write_index = 0;
static uint32_t g_input_count = 0;

void input_register_backend(const struct input_backend *backend) {
    g_input_backend = backend;
}

void input_init(void) {
    g_input_read_index = 0;
    g_input_write_index = 0;
    g_input_count = 0;

    if ((g_input_backend != NULL) && (g_input_backend->init != NULL)) {
        g_input_backend->init();
    }
}

void input_handle_platform_irq(void) {
    if ((g_input_backend != NULL) && (g_input_backend->handle_irq != NULL)) {
        g_input_backend->handle_irq();
    }
}

bool input_push_event(const struct input_event *event) {
    if ((event == NULL) || (g_input_count >= INPUT_QUEUE_CAPACITY)) {
        return false;
    }

    g_input_queue[g_input_write_index] = *event;
    g_input_write_index = (g_input_write_index + 1u) % INPUT_QUEUE_CAPACITY;
    ++g_input_count;
    return true;
}

bool input_pop_event(struct input_event *event) {
    if ((event == NULL) || (g_input_count == 0u)) {
        return false;
    }

    *event = g_input_queue[g_input_read_index];
    g_input_read_index = (g_input_read_index + 1u) % INPUT_QUEUE_CAPACITY;
    --g_input_count;
    return true;
}
