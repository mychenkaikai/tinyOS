#ifndef TINYOS_GUI_UEFI_H
#define TINYOS_GUI_UEFI_H

#include <stdbool.h>

#include "tinyos/boot_info.h"
#include "tinyos/input.h"

bool tinyos_uefi_gui_supported(const struct tinyos_boot_info *boot_info);
void tinyos_uefi_gui_init(const struct tinyos_boot_info *boot_info);
void tinyos_uefi_gui_note_heartbeat(void);
void tinyos_uefi_gui_handle_input_event(const struct input_event *event);
void tinyos_uefi_gui_render(void);

#endif
