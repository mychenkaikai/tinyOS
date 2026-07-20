#ifndef TINYOS_GUI_H
#define TINYOS_GUI_H

#include <stdint.h>

#include "tinyos/input.h"

void gui_init(void);
void gui_note_heartbeat(void);
void gui_handle_input_event(const struct input_event *event);
void gui_render(void);

#endif
