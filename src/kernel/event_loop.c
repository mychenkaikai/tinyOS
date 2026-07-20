#include "tinyos/event_loop.h"

#include <stddef.h>

#include "tinyos/arch.h"

#define EVENT_TASK_CAPACITY 8u

static struct event_task *g_tasks[EVENT_TASK_CAPACITY];
static uint32_t g_task_count = 0;

static uint64_t current_ticks(void) {
    const struct tinyos_arch_ops *arch = tinyos_arch_current();

    if ((arch == NULL) || (arch->timer_ticks == NULL)) {
        return 0u;
    }

    return arch->timer_ticks();
}

static void current_cpu_idle(void) {
    const struct tinyos_arch_ops *arch = tinyos_arch_current();

    if ((arch != NULL) && (arch->cpu_idle != NULL)) {
        arch->cpu_idle();
    }
}

void event_loop_init(void) {
    uint32_t index;

    for (index = 0; index < EVENT_TASK_CAPACITY; ++index) {
        g_tasks[index] = (struct event_task *)0;
    }

    g_task_count = 0;
}

bool event_loop_add_periodic(struct event_task *task, uint64_t period_ticks, event_task_fn handler, void *context) {
    if ((task == NULL) || (handler == NULL) || (period_ticks == 0u) || (g_task_count >= EVENT_TASK_CAPACITY)) {
        return false;
    }

    task->period_ticks = period_ticks;
    task->next_run_tick = current_ticks() + period_ticks;
    task->handler = handler;
    task->context = context;
    g_tasks[g_task_count] = task;
    ++g_task_count;
    return true;
}

void event_loop_run(void) {
    for (;;) {
        uint64_t now = current_ticks();
        bool ran_task = false;
        uint32_t index;

        for (index = 0; index < g_task_count; ++index) {
            struct event_task *task = g_tasks[index];
            if ((task != NULL) && (now >= task->next_run_tick)) {
                task->next_run_tick += task->period_ticks;
                task->handler(task->context);
                ran_task = true;
            }
        }

        if (!ran_task) {
            current_cpu_idle();
        }
    }
}
