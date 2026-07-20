#ifndef TINYOS_INPUT_H
#define TINYOS_INPUT_H

#include <stdbool.h>
#include <stdint.h>

enum input_event_type {
    INPUT_EVENT_NONE = 0,
    INPUT_EVENT_KEY = 1
};

struct input_event {
    enum input_event_type type;
    uint8_t scancode;
    char character;
    bool pressed;
};

struct input_backend {
    void (*init)(void);
    void (*handle_irq)(void);
};

void input_register_backend(const struct input_backend *backend);
void input_init(void);
void input_handle_platform_irq(void);
bool input_push_event(const struct input_event *event);
bool input_pop_event(struct input_event *event);

#endif
