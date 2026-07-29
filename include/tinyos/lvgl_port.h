#ifndef TINYOS_LVGL_PORT_H
#define TINYOS_LVGL_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "tinyos/boot_info.h"
#include "tinyos/input.h"

bool tinyos_lvgl_supported(const struct tinyos_boot_info *boot_info);
bool tinyos_lvgl_init(const struct tinyos_boot_info *boot_info);
void tinyos_lvgl_tick_inc(uint32_t milliseconds);
void tinyos_lvgl_handle_input_event(const struct input_event *event);
void tinyos_lvgl_render(void);
void tinyos_lvgl_note_heartbeat(void);

#endif
