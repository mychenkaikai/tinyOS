#include "tinyos/lvgl_port.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "tinyos/arch.h"
#include "tinyos/memory.h"

#include "lv_init.h"
#include "core/lv_group.h"
#include "core/lv_obj.h"
#include "core/lv_obj_event.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_scroll.h"
#include "core/lv_obj_style.h"
#include "display/lv_display.h"
#include "font/lv_font.h"
#include "indev/lv_indev.h"
#include "layouts/flex/lv_flex.h"
#include "misc/lv_color.h"
#include "tick/lv_tick.h"
#include "widgets/button/lv_button.h"
#include "widgets/label/lv_label.h"
#include "widgets/textarea/lv_textarea.h"

#define TINYOS_LVGL_KEY_QUEUE_CAPACITY 32u
#define TINYOS_LVGL_LAST_KEY_CAPACITY 16u
#define TINYOS_LVGL_STATUS_CAPACITY 48u
#define TINYOS_LVGL_DECIMAL_CAPACITY 24u
#define TINYOS_LVGL_PAGE_TEXT_CAPACITY 192u

enum tinyos_lvgl_page {
    TINYOS_LVGL_PAGE_HOME = 0,
    TINYOS_LVGL_PAGE_SETTINGS = 1,
    TINYOS_LVGL_PAGE_ABOUT = 2
};

enum tinyos_lvgl_action {
    TINYOS_LVGL_ACTION_HOME = 0,
    TINYOS_LVGL_ACTION_SETTINGS = 1,
    TINYOS_LVGL_ACTION_ABOUT = 2,
    TINYOS_LVGL_ACTION_CLEAR = 3
};

struct tinyos_lvgl_key_event {
    uint32_t key;
    lv_indev_state_t state;
};

struct tinyos_lvgl_state {
    bool ready;
    bool key_echo;
    volatile uint32_t *framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t pixel_format;
    uint64_t heartbeat_runs;
    uint64_t key_events;
    uint64_t render_count;
    uint64_t last_arch_tick;
    uint8_t last_scancode;
    uint8_t active_page;
    char last_key[TINYOS_LVGL_LAST_KEY_CAPACITY];
    char status[TINYOS_LVGL_STATUS_CAPACITY];
    struct tinyos_lvgl_key_event key_queue[TINYOS_LVGL_KEY_QUEUE_CAPACITY];
    uint32_t key_head;
    uint32_t key_count;
    lv_display_t *display;
    lv_indev_t *keypad;
    lv_group_t *group;
    void *draw_buffer;
    lv_obj_t *button_home;
    lv_obj_t *button_settings;
    lv_obj_t *button_about;
    lv_obj_t *button_clear;
    lv_obj_t *page_label;
    lv_obj_t *runtime_label;
    lv_obj_t *status_label;
    lv_obj_t *content_title;
    lv_obj_t *content_body;
    lv_obj_t *input_box;
};

static struct tinyos_lvgl_state g_lvgl = {
    .ready = false,
    .key_echo = true,
    .framebuffer = (volatile uint32_t *)0,
    .width = 0u,
    .height = 0u,
    .stride = 0u,
    .pixel_format = 0u,
    .heartbeat_runs = 0u,
    .key_events = 0u,
    .render_count = 0u,
    .last_arch_tick = 0u,
    .last_scancode = 0u,
    .active_page = TINYOS_LVGL_PAGE_HOME,
    .last_key = "NONE",
    .status = "LVGL UI ACTIVE",
    .key_queue = {{0}},
    .key_head = 0u,
    .key_count = 0u,
    .display = (lv_display_t *)0,
    .keypad = (lv_indev_t *)0,
    .group = (lv_group_t *)0,
    .draw_buffer = (void *)0,
    .button_home = (lv_obj_t *)0,
    .button_settings = (lv_obj_t *)0,
    .button_about = (lv_obj_t *)0,
    .button_clear = (lv_obj_t *)0,
    .page_label = (lv_obj_t *)0,
    .runtime_label = (lv_obj_t *)0,
    .status_label = (lv_obj_t *)0,
    .content_title = (lv_obj_t *)0,
    .content_body = (lv_obj_t *)0,
    .input_box = (lv_obj_t *)0
};

static uint64_t current_ticks(void) {
    const struct tinyos_arch_ops *arch = tinyos_arch_current();

    if ((arch == NULL) || (arch->timer_ticks == NULL)) {
        return 0u;
    }

    return arch->timer_ticks();
}

static void copy_string(char *destination, uint32_t capacity, const char *source) {
    uint32_t index = 0u;

    if ((destination == NULL) || (capacity == 0u)) {
        return;
    }

    while ((source != NULL) && (source[index] != '\0') && (index + 1u < capacity)) {
        destination[index] = source[index];
        ++index;
    }

    destination[index] = '\0';
}

static uint32_t string_length(const char *text) {
    uint32_t length = 0u;

    while ((text != NULL) && (text[length] != '\0')) {
        ++length;
    }

    return length;
}

static void append_string(char *destination, uint32_t capacity, const char *source) {
    uint32_t index = string_length(destination);
    uint32_t offset = 0u;

    if ((destination == NULL) || (capacity == 0u) || (index >= capacity)) {
        return;
    }

    while ((source != NULL) && (source[offset] != '\0') && (index + 1u < capacity)) {
        destination[index++] = source[offset++];
    }

    destination[index] = '\0';
}

static void uint64_to_decimal(uint64_t value, char *buffer, uint32_t capacity) {
    char digits[20];
    uint32_t count = 0u;
    uint32_t index = 0u;

    if ((buffer == NULL) || (capacity == 0u)) {
        return;
    }

    if (value == 0u) {
        if (capacity > 1u) {
            buffer[0] = '0';
            buffer[1] = '\0';
        } else {
            buffer[0] = '\0';
        }
        return;
    }

    while ((value != 0u) && (count < (uint32_t)sizeof(digits))) {
        digits[count] = (char)('0' + (value % 10u));
        value /= 10u;
        ++count;
    }

    while ((count > 0u) && (index + 1u < capacity)) {
        --count;
        buffer[index++] = digits[count];
    }

    buffer[index] = '\0';
}

static void uint8_to_hex(uint8_t value, char *buffer, uint32_t capacity) {
    static const char digits[] = "0123456789ABCDEF";

    if ((buffer == NULL) || (capacity < 5u)) {
        return;
    }

    buffer[0] = '0';
    buffer[1] = 'X';
    buffer[2] = digits[(value >> 4u) & 0x0Fu];
    buffer[3] = digits[value & 0x0Fu];
    buffer[4] = '\0';
}

static bool is_printable_ascii(char ch) {
    return (ch >= 32) && (ch <= 126);
}

static char upper_ascii(char ch) {
    if ((ch >= 'a') && (ch <= 'z')) {
        return (char)(ch - ('a' - 'A'));
    }

    return ch;
}

static const char *page_name(uint8_t page) {
    if (page == TINYOS_LVGL_PAGE_SETTINGS) {
        return "SETTINGS";
    }

    if (page == TINYOS_LVGL_PAGE_ABOUT) {
        return "ABOUT";
    }

    return "HOME";
}

static void set_status(const char *message) {
    copy_string(g_lvgl.status, TINYOS_LVGL_STATUS_CAPACITY, message);
}

static void set_last_key_label(char ch) {
    if (ch == '\n') {
        copy_string(g_lvgl.last_key, TINYOS_LVGL_LAST_KEY_CAPACITY, "ENTER");
        return;
    }

    if (ch == '\t') {
        copy_string(g_lvgl.last_key, TINYOS_LVGL_LAST_KEY_CAPACITY, "TAB");
        return;
    }

    if (ch == '\b') {
        copy_string(g_lvgl.last_key, TINYOS_LVGL_LAST_KEY_CAPACITY, "BACK");
        return;
    }

    if (ch == ' ') {
        copy_string(g_lvgl.last_key, TINYOS_LVGL_LAST_KEY_CAPACITY, "SPACE");
        return;
    }

    if (is_printable_ascii(ch)) {
        g_lvgl.last_key[0] = upper_ascii(ch);
        g_lvgl.last_key[1] = '\0';
        return;
    }

    copy_string(g_lvgl.last_key, TINYOS_LVGL_LAST_KEY_CAPACITY, "OTHER");
}

static uint32_t convert_pixel_to_framebuffer(uint32_t pixel) {
    if (g_lvgl.pixel_format == 1u) {
        return pixel & 0x00FFFFFFu;
    }

    return ((pixel & 0x00FF0000u) >> 16u) | (pixel & 0x0000FF00u) | ((pixel & 0x000000FFu) << 16u);
}

static void clear_framebuffer(uint32_t color) {
    uint32_t x;
    uint32_t y;

    if (g_lvgl.framebuffer == (volatile uint32_t *)0) {
        return;
    }

    for (y = 0u; y < g_lvgl.height; ++y) {
        volatile uint32_t *row = g_lvgl.framebuffer + (y * g_lvgl.stride);
        for (x = 0u; x < g_lvgl.width; ++x) {
            row[x] = color;
        }
    }
}

static void clear_draw_buffer(uint32_t color) {
    uint32_t *buffer = (uint32_t *)g_lvgl.draw_buffer;
    uint32_t pixel_count;
    uint32_t index;

    if (buffer == (uint32_t *)0) {
        return;
    }

    pixel_count = g_lvgl.width * g_lvgl.height;
    for (index = 0u; index < pixel_count; ++index) {
        buffer[index] = color;
    }
}

static void flush_display(lv_display_t *display, const lv_area_t *area, uint8_t *px_map) {
    int32_t clipped_x1;
    int32_t clipped_y1;
    int32_t clipped_x2;
    int32_t clipped_y2;
    int32_t x;
    int32_t y;
    uint32_t source_row_stride;
    const uint32_t *source = (const uint32_t *)px_map;
    uint32_t source_x_offset;
    uint32_t source_y_offset;

    (void)display;

    if ((area == NULL) || (px_map == NULL) || (g_lvgl.framebuffer == (volatile uint32_t *)0)) {
        lv_display_flush_ready(g_lvgl.display);
        return;
    }

    clipped_x1 = (area->x1 < 0) ? 0 : area->x1;
    clipped_y1 = (area->y1 < 0) ? 0 : area->y1;
    clipped_x2 = (area->x2 >= (int32_t)g_lvgl.width) ? (int32_t)g_lvgl.width - 1 : area->x2;
    clipped_y2 = (area->y2 >= (int32_t)g_lvgl.height) ? (int32_t)g_lvgl.height - 1 : area->y2;

    if ((clipped_x1 > clipped_x2) || (clipped_y1 > clipped_y2)) {
        lv_display_flush_ready(g_lvgl.display);
        return;
    }

    source_row_stride = (uint32_t)(area->x2 - area->x1 + 1);
    source_x_offset = (uint32_t)(clipped_x1 - area->x1);
    source_y_offset = (uint32_t)(clipped_y1 - area->y1);
    for (y = clipped_y1; y <= clipped_y2; ++y) {
        volatile uint32_t *destination = g_lvgl.framebuffer + ((uint32_t)y * g_lvgl.stride) + (uint32_t)clipped_x1;
        const uint32_t *line = source + ((source_y_offset + (uint32_t)(y - clipped_y1)) * source_row_stride);

        for (x = clipped_x1; x <= clipped_x2; ++x) {
            destination[x - clipped_x1] = convert_pixel_to_framebuffer(line[source_x_offset + (uint32_t)(x - clipped_x1)]);
        }
    }

    lv_display_flush_ready(g_lvgl.display);
}

static bool queue_key_event(uint32_t key, lv_indev_state_t state) {
    uint32_t tail;

    if (g_lvgl.key_count >= TINYOS_LVGL_KEY_QUEUE_CAPACITY) {
        return false;
    }

    tail = (g_lvgl.key_head + g_lvgl.key_count) % TINYOS_LVGL_KEY_QUEUE_CAPACITY;
    g_lvgl.key_queue[tail].key = key;
    g_lvgl.key_queue[tail].state = state;
    ++g_lvgl.key_count;
    return true;
}

static void keypad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    (void)indev;

    if ((data == NULL) || (g_lvgl.key_count == 0u)) {
        if (data != NULL) {
            data->key = 0u;
            data->state = LV_INDEV_STATE_RELEASED;
            data->continue_reading = false;
        }
        return;
    }

    data->key = g_lvgl.key_queue[g_lvgl.key_head].key;
    data->state = g_lvgl.key_queue[g_lvgl.key_head].state;
    g_lvgl.key_head = (g_lvgl.key_head + 1u) % TINYOS_LVGL_KEY_QUEUE_CAPACITY;
    --g_lvgl.key_count;
    data->continue_reading = g_lvgl.key_count > 0u;
}

static void update_page_focus(void) {
    if (g_lvgl.button_home == (lv_obj_t *)0) {
        return;
    }

    lv_obj_remove_state(g_lvgl.button_home, LV_STATE_CHECKED);
    lv_obj_remove_state(g_lvgl.button_settings, LV_STATE_CHECKED);
    lv_obj_remove_state(g_lvgl.button_about, LV_STATE_CHECKED);

    if (g_lvgl.active_page == TINYOS_LVGL_PAGE_SETTINGS) {
        lv_obj_add_state(g_lvgl.button_settings, LV_STATE_CHECKED);
        return;
    }

    if (g_lvgl.active_page == TINYOS_LVGL_PAGE_ABOUT) {
        lv_obj_add_state(g_lvgl.button_about, LV_STATE_CHECKED);
        return;
    }

    lv_obj_add_state(g_lvgl.button_home, LV_STATE_CHECKED);
}

static void update_content_labels(void) {
    char body[TINYOS_LVGL_PAGE_TEXT_CAPACITY];

    if (!g_lvgl.ready) {
        return;
    }

    copy_string(body, (uint32_t)sizeof(body), "");
    if (g_lvgl.active_page == TINYOS_LVGL_PAGE_SETTINGS) {
        lv_label_set_text(g_lvgl.content_title, "SETTINGS");
        append_string(body, (uint32_t)sizeof(body), "KEY ECHO: ");
        append_string(body, (uint32_t)sizeof(body), g_lvgl.key_echo ? "ON" : "OFF");
        append_string(body, (uint32_t)sizeof(body), "\nPRESS 2 OR ENTER ON SETTINGS TO TOGGLE");
        append_string(body, (uint32_t)sizeof(body), "\nTAB MOVES BUTTON FOCUS");
        append_string(body, (uint32_t)sizeof(body), "\n0 OR C CLEARS THE INPUT BOX");
    } else if (g_lvgl.active_page == TINYOS_LVGL_PAGE_ABOUT) {
        lv_label_set_text(g_lvgl.content_title, "ABOUT");
        append_string(body, (uint32_t)sizeof(body), "LVGL V9 IS NOW DRIVING THE UEFI UI.");
        append_string(body, (uint32_t)sizeof(body), "\nDISPLAY FLUSH, KEYBOARD INPUT AND TIMER");
        append_string(body, (uint32_t)sizeof(body), "\nARE WIRED INTO THE KERNEL EVENT LOOP.");
        append_string(body, (uint32_t)sizeof(body), "\nTHE OLD BUILTIN FRAMEBUFFER UI REMAINS");
        append_string(body, (uint32_t)sizeof(body), "\nAS THE FALLBACK PATH.");
    } else {
        lv_label_set_text(g_lvgl.content_title, "DASHBOARD");
        append_string(body, (uint32_t)sizeof(body), "UEFI GOP FRAMEBUFFER ACTIVE");
        append_string(body, (uint32_t)sizeof(body), "\nLVGL SW RENDERER ACTIVE");
        append_string(body, (uint32_t)sizeof(body), "\nTASK LOOP ACTIVE");
        append_string(body, (uint32_t)sizeof(body), "\nHOTKEYS: 1 HOME  2 SETTINGS  3 ABOUT");
        append_string(body, (uint32_t)sizeof(body), "\nTAB NEXT  ENTER OPEN  0 CLEAR");
    }

    lv_label_set_text(g_lvgl.content_body, body);
    lv_label_set_text_fmt(g_lvgl.page_label, "PAGE %s", page_name(g_lvgl.active_page));
    lv_label_set_text_fmt(g_lvgl.status_label, "STATUS %s", g_lvgl.status);
    update_page_focus();
}

static void update_runtime_label(void) {
    char ticks_text[TINYOS_LVGL_DECIMAL_CAPACITY];
    char heart_text[TINYOS_LVGL_DECIMAL_CAPACITY];
    char events_text[TINYOS_LVGL_DECIMAL_CAPACITY];
    char heap_text[TINYOS_LVGL_DECIMAL_CAPACITY];
    char scan_text[8];
    char runtime_text[192];

    uint64_to_decimal(current_ticks(), ticks_text, (uint32_t)sizeof(ticks_text));
    uint64_to_decimal(g_lvgl.heartbeat_runs, heart_text, (uint32_t)sizeof(heart_text));
    uint64_to_decimal(g_lvgl.key_events, events_text, (uint32_t)sizeof(events_text));
    uint64_to_decimal((uint64_t)memory_bytes_used(), heap_text, (uint32_t)sizeof(heap_text));
    uint8_to_hex(g_lvgl.last_scancode, scan_text, (uint32_t)sizeof(scan_text));

    copy_string(runtime_text, (uint32_t)sizeof(runtime_text), "RUNTIME\n");
    append_string(runtime_text, (uint32_t)sizeof(runtime_text), "TICKS ");
    append_string(runtime_text, (uint32_t)sizeof(runtime_text), ticks_text);
    append_string(runtime_text, (uint32_t)sizeof(runtime_text), "\nHEART ");
    append_string(runtime_text, (uint32_t)sizeof(runtime_text), heart_text);
    append_string(runtime_text, (uint32_t)sizeof(runtime_text), "\nEVENTS ");
    append_string(runtime_text, (uint32_t)sizeof(runtime_text), events_text);
    append_string(runtime_text, (uint32_t)sizeof(runtime_text), "\nLAST ");
    append_string(runtime_text, (uint32_t)sizeof(runtime_text), g_lvgl.last_key);
    append_string(runtime_text, (uint32_t)sizeof(runtime_text), "\nSCAN ");
    append_string(runtime_text, (uint32_t)sizeof(runtime_text), scan_text);
    append_string(runtime_text, (uint32_t)sizeof(runtime_text), "\nHEAP ");
    append_string(runtime_text, (uint32_t)sizeof(runtime_text), heap_text);
    append_string(runtime_text, (uint32_t)sizeof(runtime_text), "\nDISPLAY ");
    append_string(runtime_text, (uint32_t)sizeof(runtime_text), (g_lvgl.pixel_format == 1u) ? "BGR" : "RGB");

    lv_label_set_text(g_lvgl.runtime_label, runtime_text);
}

static void clear_input_box(void) {
    if (g_lvgl.input_box != (lv_obj_t *)0) {
        lv_textarea_set_text(g_lvgl.input_box, "");
    }
}

static void handle_action(enum tinyos_lvgl_action action) {
    if (action == TINYOS_LVGL_ACTION_SETTINGS) {
        g_lvgl.active_page = TINYOS_LVGL_PAGE_SETTINGS;
        g_lvgl.key_echo = !g_lvgl.key_echo;
        set_status(g_lvgl.key_echo ? "KEY ECHO ENABLED" : "KEY ECHO DISABLED");
    } else if (action == TINYOS_LVGL_ACTION_ABOUT) {
        g_lvgl.active_page = TINYOS_LVGL_PAGE_ABOUT;
        set_status("ABOUT PAGE ACTIVE");
    } else if (action == TINYOS_LVGL_ACTION_CLEAR) {
        clear_input_box();
        set_status("INPUT CLEARED");
    } else {
        g_lvgl.active_page = TINYOS_LVGL_PAGE_HOME;
        set_status("HOME PAGE ACTIVE");
    }

    update_content_labels();
}

static void button_event_cb(lv_event_t *event) {
    uintptr_t action_value;

    if ((event == NULL) || (lv_event_get_code(event) != LV_EVENT_CLICKED)) {
        return;
    }

    action_value = (uintptr_t)lv_event_get_user_data(event);
    handle_action((enum tinyos_lvgl_action)action_value);
}

static lv_obj_t *create_nav_button(lv_obj_t *parent, const char *label, enum tinyos_lvgl_action action) {
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_t *button_label = lv_label_create(button);

    lv_obj_set_flex_grow(button, 1);
    lv_obj_add_event_cb(button, button_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)action);
    lv_label_set_text(button_label, label);
    lv_obj_center(button_label);
    return button;
}

static void build_ui(void) {
    lv_obj_t *screen = lv_screen_active();
    lv_obj_t *root = lv_obj_create(screen);
    lv_obj_t *top_bar = lv_obj_create(root);
    lv_obj_t *title = lv_label_create(top_bar);
    lv_obj_t *nav_row = lv_obj_create(root);
    lv_obj_t *middle_row = lv_obj_create(root);
    lv_obj_t *runtime_panel = lv_obj_create(middle_row);
    lv_obj_t *content_panel = lv_obj_create(middle_row);
    lv_obj_t *input_panel = lv_obj_create(root);
    lv_obj_t *input_label = lv_label_create(input_panel);
    lv_obj_t *footer = lv_label_create(root);

    lv_obj_set_style_bg_color(screen, lv_color_hex(0x060E20), 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(0xE8F1FF), 0);
    lv_obj_set_style_border_width(screen, 0, 0);

    lv_obj_set_size(root, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(root, LV_OPA_0, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 16, 0);
    lv_obj_set_style_pad_gap(root, 12, 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(root, LV_SCROLLBAR_MODE_OFF);

    lv_obj_set_width(top_bar, lv_pct(100));
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x3AAAF0), 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_pad_all(top_bar, 14, 0);
    lv_obj_set_style_radius(top_bar, 8, 0);

    lv_label_set_text(title, "TINYOS");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xF7FBFF), 0);

    g_lvgl.page_label = lv_label_create(top_bar);
    lv_obj_align(g_lvgl.page_label, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_set_width(nav_row, lv_pct(100));
    lv_obj_set_style_bg_opa(nav_row, LV_OPA_0, 0);
    lv_obj_set_style_border_width(nav_row, 0, 0);
    lv_obj_set_style_pad_all(nav_row, 0, 0);
    lv_obj_set_style_pad_gap(nav_row, 10, 0);
    lv_obj_set_flex_flow(nav_row, LV_FLEX_FLOW_ROW);

    g_lvgl.button_home = create_nav_button(nav_row, "HOME", TINYOS_LVGL_ACTION_HOME);
    g_lvgl.button_settings = create_nav_button(nav_row, "SETTINGS", TINYOS_LVGL_ACTION_SETTINGS);
    g_lvgl.button_about = create_nav_button(nav_row, "ABOUT", TINYOS_LVGL_ACTION_ABOUT);
    g_lvgl.button_clear = create_nav_button(nav_row, "CLEAR", TINYOS_LVGL_ACTION_CLEAR);

    lv_obj_set_size(middle_row, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_grow(middle_row, 1);
    lv_obj_set_style_bg_opa(middle_row, LV_OPA_0, 0);
    lv_obj_set_style_border_width(middle_row, 0, 0);
    lv_obj_set_style_pad_all(middle_row, 0, 0);
    lv_obj_set_style_pad_gap(middle_row, 12, 0);
    lv_obj_set_flex_flow(middle_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_scrollbar_mode(middle_row, LV_SCROLLBAR_MODE_OFF);

    lv_obj_set_width(runtime_panel, lv_pct(32));
    lv_obj_set_flex_grow(runtime_panel, 0);
    lv_obj_set_style_bg_color(runtime_panel, lv_color_hex(0x182A48), 0);
    lv_obj_set_style_border_color(runtime_panel, lv_color_hex(0x3AAAF0), 0);
    lv_obj_set_style_border_width(runtime_panel, 2, 0);
    lv_obj_set_style_radius(runtime_panel, 8, 0);
    lv_obj_set_style_pad_all(runtime_panel, 14, 0);
    lv_obj_set_scrollbar_mode(runtime_panel, LV_SCROLLBAR_MODE_OFF);

    g_lvgl.runtime_label = lv_label_create(runtime_panel);
    lv_obj_set_width(g_lvgl.runtime_label, lv_pct(100));

    lv_obj_set_flex_grow(content_panel, 1);
    lv_obj_set_style_bg_color(content_panel, lv_color_hex(0x0F1C36), 0);
    lv_obj_set_style_border_color(content_panel, lv_color_hex(0x3AAAF0), 0);
    lv_obj_set_style_border_width(content_panel, 2, 0);
    lv_obj_set_style_radius(content_panel, 8, 0);
    lv_obj_set_style_pad_all(content_panel, 18, 0);
    lv_obj_set_style_pad_gap(content_panel, 12, 0);
    lv_obj_set_flex_flow(content_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(content_panel, LV_SCROLLBAR_MODE_OFF);

    g_lvgl.content_title = lv_label_create(content_panel);
    lv_obj_set_style_text_font(g_lvgl.content_title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(g_lvgl.content_title, lv_color_hex(0x3AAAF0), 0);

    g_lvgl.content_body = lv_label_create(content_panel);
    lv_obj_set_width(g_lvgl.content_body, lv_pct(100));

    lv_obj_set_width(input_panel, lv_pct(100));
    lv_obj_set_style_bg_color(input_panel, lv_color_hex(0x182A48), 0);
    lv_obj_set_style_border_color(input_panel, lv_color_hex(0x3AAAF0), 0);
    lv_obj_set_style_border_width(input_panel, 2, 0);
    lv_obj_set_style_radius(input_panel, 8, 0);
    lv_obj_set_style_pad_all(input_panel, 14, 0);
    lv_obj_set_style_pad_gap(input_panel, 10, 0);
    lv_obj_set_flex_flow(input_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(input_panel, LV_SCROLLBAR_MODE_OFF);

    lv_label_set_text(input_label, "INPUT");
    lv_obj_set_style_text_font(input_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(input_label, lv_color_hex(0x3AAAF0), 0);

    g_lvgl.input_box = lv_textarea_create(input_panel);
    lv_obj_set_width(g_lvgl.input_box, lv_pct(100));
    lv_textarea_set_one_line(g_lvgl.input_box, true);
    lv_textarea_set_max_length(g_lvgl.input_box, 24u);
    lv_textarea_set_placeholder_text(g_lvgl.input_box, "TYPE ON KEYBOARD");

    g_lvgl.status_label = lv_label_create(root);
    lv_obj_set_width(g_lvgl.status_label, lv_pct(100));

    lv_label_set_text(footer, "TAB NEXT  ENTER OPEN  123 PAGES  0 CLEAR");
    lv_obj_set_style_text_color(footer, lv_color_hex(0x9AB4D6), 0);

    if (g_lvgl.group != (lv_group_t *)0) {
        lv_group_add_obj(g_lvgl.group, g_lvgl.button_home);
        lv_group_add_obj(g_lvgl.group, g_lvgl.button_settings);
        lv_group_add_obj(g_lvgl.group, g_lvgl.button_about);
        lv_group_add_obj(g_lvgl.group, g_lvgl.button_clear);
        lv_group_focus_obj(g_lvgl.button_home);
    }

    update_content_labels();
    update_runtime_label();
}

static void sync_ticks(void) {
    uint64_t now = current_ticks();

    if (g_lvgl.last_arch_tick == 0u) {
        g_lvgl.last_arch_tick = now;
        return;
    }

    if (now > g_lvgl.last_arch_tick) {
        tinyos_lvgl_tick_inc((uint32_t)((now - g_lvgl.last_arch_tick) * 10u));
        g_lvgl.last_arch_tick = now;
    }
}

static uint32_t map_key(const struct input_event *event) {
    if ((event == (const struct input_event *)0) || (event->type != INPUT_EVENT_KEY)) {
        return 0u;
    }

    if (event->character == '\t') {
        return LV_KEY_NEXT;
    }

    if (event->character == '\n') {
        return LV_KEY_ENTER;
    }

    if (event->character == '\b') {
        return LV_KEY_BACKSPACE;
    }

    return 0u;
}

bool tinyos_lvgl_supported(const struct tinyos_boot_info *boot_info) {
    if ((boot_info == (const struct tinyos_boot_info *)0) ||
        (boot_info->boot_method != TINYOS_BOOT_METHOD_UEFI) ||
        !boot_info->framebuffer.available ||
        (boot_info->framebuffer.base == 0u)) {
        return false;
    }

    return (boot_info->framebuffer.width >= 640u) &&
           (boot_info->framebuffer.height >= 360u) &&
           (boot_info->framebuffer.pixels_per_scanline >= boot_info->framebuffer.width);
}

bool tinyos_lvgl_init(const struct tinyos_boot_info *boot_info) {
    uint32_t draw_buffer_size;

    if (!tinyos_lvgl_supported(boot_info)) {
        return false;
    }

    g_lvgl.framebuffer = (volatile uint32_t *)(uintptr_t)boot_info->framebuffer.base;
    g_lvgl.width = boot_info->framebuffer.width;
    g_lvgl.height = boot_info->framebuffer.height;
    g_lvgl.stride = boot_info->framebuffer.pixels_per_scanline;
    g_lvgl.pixel_format = boot_info->framebuffer.pixel_format;
    g_lvgl.heartbeat_runs = 0u;
    g_lvgl.key_events = 0u;
    g_lvgl.render_count = 0u;
    g_lvgl.last_arch_tick = current_ticks();
    g_lvgl.last_scancode = 0u;
    g_lvgl.active_page = TINYOS_LVGL_PAGE_HOME;
    g_lvgl.key_echo = true;
    copy_string(g_lvgl.last_key, TINYOS_LVGL_LAST_KEY_CAPACITY, "NONE");
    copy_string(g_lvgl.status, TINYOS_LVGL_STATUS_CAPACITY, "LVGL UI ACTIVE");
    g_lvgl.key_head = 0u;
    g_lvgl.key_count = 0u;
    clear_framebuffer(convert_pixel_to_framebuffer(0x060E20u));

    lv_init();

    g_lvgl.display = lv_display_create((int32_t)g_lvgl.width, (int32_t)g_lvgl.height);
    if (g_lvgl.display == (lv_display_t *)0) {
        return false;
    }

    draw_buffer_size = g_lvgl.width * g_lvgl.height * sizeof(uint32_t);
    g_lvgl.draw_buffer = memory_alloc(draw_buffer_size, 64u);
    if (g_lvgl.draw_buffer == (void *)0) {
        return false;
    }

    lv_display_set_color_format(g_lvgl.display, LV_COLOR_FORMAT_XRGB8888);
    lv_display_set_buffers(
        g_lvgl.display,
        g_lvgl.draw_buffer,
        (void *)0,
        draw_buffer_size,
        LV_DISPLAY_RENDER_MODE_FULL
    );
    lv_display_set_flush_cb(g_lvgl.display, flush_display);
    lv_display_set_default(g_lvgl.display);

    g_lvgl.group = lv_group_create();
    g_lvgl.keypad = lv_indev_create();
    if ((g_lvgl.group == (lv_group_t *)0) || (g_lvgl.keypad == (lv_indev_t *)0)) {
        return false;
    }

    lv_indev_set_type(g_lvgl.keypad, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(g_lvgl.keypad, keypad_read);
    lv_indev_set_display(g_lvgl.keypad, g_lvgl.display);
    lv_indev_set_group(g_lvgl.keypad, g_lvgl.group);
    lv_group_set_wrap(g_lvgl.group, true);

    build_ui();
    g_lvgl.ready = true;
    tinyos_lvgl_render();
    return true;
}

void tinyos_lvgl_tick_inc(uint32_t milliseconds) {
    if (!g_lvgl.ready || (milliseconds == 0u)) {
        return;
    }

    lv_tick_inc(milliseconds);
}

void tinyos_lvgl_handle_input_event(const struct input_event *event) {
    uint32_t mapped_key;
    char text[2];

    if (!g_lvgl.ready || (event == (const struct input_event *)0) || (event->type != INPUT_EVENT_KEY)) {
        return;
    }

    if (event->pressed) {
        ++g_lvgl.key_events;
        g_lvgl.last_scancode = event->scancode;
        set_last_key_label(event->character);
    }

    if (!event->pressed) {
        mapped_key = map_key(event);
        if (mapped_key != 0u) {
            (void)queue_key_event(mapped_key, LV_INDEV_STATE_RELEASED);
        }
        return;
    }

    if (event->character == '1') {
        handle_action(TINYOS_LVGL_ACTION_HOME);
        return;
    }

    if (event->character == '2') {
        handle_action(TINYOS_LVGL_ACTION_SETTINGS);
        return;
    }

    if (event->character == '3') {
        handle_action(TINYOS_LVGL_ACTION_ABOUT);
        return;
    }

    if ((event->character == '0') || (event->character == 'c') || (event->character == 'C')) {
        handle_action(TINYOS_LVGL_ACTION_CLEAR);
        return;
    }

    if (event->character == '\b') {
        lv_textarea_delete_char(g_lvgl.input_box);
        set_status("INPUT ERASED");
        update_content_labels();
        return;
    }

    if (g_lvgl.key_echo && is_printable_ascii(event->character)) {
        text[0] = upper_ascii(event->character);
        text[1] = '\0';
        lv_textarea_add_text(g_lvgl.input_box, text);
        set_status("INPUT UPDATED");
        update_content_labels();
        return;
    }

    mapped_key = map_key(event);
    if (mapped_key != 0u) {
        if (queue_key_event(mapped_key, LV_INDEV_STATE_PRESSED)) {
            set_status((mapped_key == LV_KEY_NEXT) ? "FOCUS MOVED" : "BUTTON ACTIVATED");
            update_content_labels();
        }
        return;
    }

    set_status("IGNORED KEY");
    update_content_labels();
}

void tinyos_lvgl_render(void) {
    if (!g_lvgl.ready) {
        return;
    }

    ++g_lvgl.render_count;
    sync_ticks();
    update_runtime_label();
    update_content_labels();
    clear_draw_buffer(0x060E20u);
    lv_obj_invalidate(lv_screen_active());
    lv_timer_handler();
}

void tinyos_lvgl_note_heartbeat(void) {
    if (!g_lvgl.ready) {
        return;
    }

    ++g_lvgl.heartbeat_runs;
}
