#ifndef TINYOS_EVENT_LOOP_H
#define TINYOS_EVENT_LOOP_H

#include <stdbool.h>
#include <stdint.h>

typedef void (*event_task_fn)(void *context);

struct event_task {
    uint64_t period_ticks;
    uint64_t next_run_tick;
    event_task_fn handler;
    void *context;
};

void event_loop_init(void);
bool event_loop_add_periodic(struct event_task *task, uint64_t period_ticks, event_task_fn handler, void *context);
void event_loop_run(void) __attribute__((noreturn));

#endif
