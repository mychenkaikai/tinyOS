#include "tinyos/gui.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "tinyos/arch.h"
#include "tinyos/display.h"

#define GUI_INPUT_CAPACITY 32u
#define GUI_LAST_KEY_CAPACITY 24u
#define GUI_STATUS_CAPACITY 48u

enum gui_focus_target {
    GUI_FOCUS_HOME = 0,
    GUI_FOCUS_SETTINGS = 1,
    GUI_FOCUS_CLEAR = 2,
    GUI_FOCUS_COUNT = 3
};

enum gui_page {
    GUI_PAGE_HOME = 0,
    GUI_PAGE_SETTINGS = 1
};

struct gui_state {
    bool ready;
    uint32_t width;
    uint32_t height;
    uint64_t heartbeat_runs;
    uint64_t key_events;
    uint8_t focus_index;
    uint8_t active_page;
    bool desktop_details;
    bool settings_key_echo;
    uint8_t last_scancode;
    char last_key[GUI_LAST_KEY_CAPACITY];
    char status_message[GUI_STATUS_CAPACITY];
    char input_buffer[GUI_INPUT_CAPACITY + 1u];
    uint32_t input_length;
};

static struct gui_state g_gui = {
    .ready = false,
    .width = 80u,
    .height = 25u,
    .heartbeat_runs = 0u,
    .key_events = 0u,
    .focus_index = GUI_FOCUS_HOME,
    .active_page = GUI_PAGE_HOME,
    .desktop_details = false,
    .settings_key_echo = true,
    .last_scancode = 0u,
    .last_key = "none",
    .status_message = "GUI waiting for keyboard input.",
    .input_buffer = {0},
    .input_length = 0u
};

static uint64_t current_ticks(void) {
    const struct tinyos_arch_ops *arch = tinyos_arch_current();

    if ((arch == NULL) || (arch->timer_ticks == NULL)) {
        return 0u;
    }

    return arch->timer_ticks();
}

static uint32_t string_length(const char *text) {
    uint32_t length = 0u;

    while ((text != NULL) && (text[length] != '\0')) {
        ++length;
    }

    return length;
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
        buffer[index] = digits[count];
        ++index;
    }

    buffer[index] = '\0';
}

static void uint8_to_hex(uint8_t value, char *buffer, uint32_t capacity) {
    static const char digits[] = "0123456789ABCDEF";

    if ((buffer == NULL) || (capacity < 5u)) {
        return;
    }

    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[2] = digits[(value >> 4) & 0x0Fu];
    buffer[3] = digits[value & 0x0Fu];
    buffer[4] = '\0';
}

static bool is_printable_ascii(char ch) {
    return (ch >= 32) && (ch <= 126);
}

static const char *page_name(uint8_t page) {
    return (page == GUI_PAGE_SETTINGS) ? "SETTINGS" : "HOME";
}

static const char *focus_name(uint8_t focus) {
    if (focus == GUI_FOCUS_SETTINGS) {
        return "SETTINGS";
    }

    if (focus == GUI_FOCUS_CLEAR) {
        return "CLEAR";
    }

    return "HOME";
}

static void set_status_message(const char *message) {
    copy_string(g_gui.status_message, GUI_STATUS_CAPACITY, message);
}

static void set_last_key_label(char ch) {
    if (ch == '\n') {
        copy_string(g_gui.last_key, GUI_LAST_KEY_CAPACITY, "enter");
        return;
    }

    if (ch == '\t') {
        copy_string(g_gui.last_key, GUI_LAST_KEY_CAPACITY, "tab");
        return;
    }

    if (ch == '\b') {
        copy_string(g_gui.last_key, GUI_LAST_KEY_CAPACITY, "backspace");
        return;
    }

    if (ch == ' ') {
        copy_string(g_gui.last_key, GUI_LAST_KEY_CAPACITY, "space");
        return;
    }

    if (is_printable_ascii(ch)) {
        g_gui.last_key[0] = ch;
        g_gui.last_key[1] = '\0';
        return;
    }

    copy_string(g_gui.last_key, GUI_LAST_KEY_CAPACITY, "non-printable");
}

static void draw_char(uint32_t row, uint32_t col, char ch) {
    if (!g_gui.ready) {
        return;
    }

    if ((row >= g_gui.height) || (col >= g_gui.width)) {
        return;
    }

    (void)display_write_at(row, col, ch);
}

static void draw_text(uint32_t row, uint32_t col, const char *text) {
    uint32_t index = 0u;

    while ((text != NULL) && (text[index] != '\0') && (col + index < g_gui.width)) {
        draw_char(row, col + index, text[index]);
        ++index;
    }
}

static void draw_blank_line(uint32_t row) {
    uint32_t col;

    for (col = 0u; col < g_gui.width; ++col) {
        draw_char(row, col, ' ');
    }
}

static void clear_screen(void) {
    uint32_t row;

    for (row = 0u; row < g_gui.height; ++row) {
        draw_blank_line(row);
    }
}

static void draw_box(uint32_t top, uint32_t left, uint32_t width, uint32_t height, const char *title) {
    uint32_t row;
    uint32_t col;

    if ((width < 2u) || (height < 2u)) {
        return;
    }

    for (col = 0u; col < width; ++col) {
        draw_char(top, left + col, (col == 0u || col == width - 1u) ? '+' : '-');
        draw_char(top + height - 1u, left + col, (col == 0u || col == width - 1u) ? '+' : '-');
    }

    for (row = 1u; row + 1u < height; ++row) {
        draw_char(top + row, left, '|');
        draw_char(top + row, left + width - 1u, '|');
    }

    if ((title != NULL) && (string_length(title) + 4u < width)) {
        draw_text(top, left + 2u, title);
    }
}

static void draw_kv_line(uint32_t row, const char *label, const char *value) {
    draw_text(row, 2u, label);
    draw_text(row, 15u, value);
}

static void draw_button(uint32_t row, uint32_t col, const char *label, bool focused) {
    draw_char(row, col, focused ? '>' : '[');
    draw_text(row, col + 1u, label);
    draw_char(row, col + 1u + string_length(label), focused ? '<' : ']');
}

static void append_input_char(char ch) {
    if (g_gui.input_length >= GUI_INPUT_CAPACITY) {
        set_status_message("Input buffer full. Use Clear Input.");
        return;
    }

    g_gui.input_buffer[g_gui.input_length] = ch;
    ++g_gui.input_length;
    g_gui.input_buffer[g_gui.input_length] = '\0';

    if (g_gui.settings_key_echo) {
        set_status_message("Printable key appended to demo input.");
    } else {
        set_status_message("Input updated.");
    }
}

static void erase_input_char(void) {
    if (g_gui.input_length == 0u) {
        set_status_message("Input box is already empty.");
        return;
    }

    --g_gui.input_length;
    g_gui.input_buffer[g_gui.input_length] = '\0';
    set_status_message("Removed one character from demo input.");
}

static void activate_focus(void) {
    if (g_gui.focus_index == GUI_FOCUS_HOME) {
        if (g_gui.active_page == GUI_PAGE_HOME) {
            g_gui.desktop_details = !g_gui.desktop_details;
            set_status_message(g_gui.desktop_details ? "Desktop detail cards enabled." : "Desktop detail cards hidden.");
        } else {
            g_gui.active_page = GUI_PAGE_HOME;
            set_status_message("Switched to Home page.");
        }
        return;
    }

    if (g_gui.focus_index == GUI_FOCUS_SETTINGS) {
        if (g_gui.active_page == GUI_PAGE_SETTINGS) {
            g_gui.settings_key_echo = !g_gui.settings_key_echo;
            set_status_message(g_gui.settings_key_echo ? "Settings: key echo enabled." : "Settings: key echo disabled.");
        } else {
            g_gui.active_page = GUI_PAGE_SETTINGS;
            set_status_message("Switched to Settings page.");
        }
        return;
    }

    g_gui.input_length = 0u;
    g_gui.input_buffer[0] = '\0';
    set_status_message("Demo input cleared.");
}

static void render_header(void) {
    char ticks_text[24];

    draw_box(0u, 0u, g_gui.width, 5u, " tinyOS GUI MVP ");
    draw_text(1u, 2u, "Page:");
    draw_text(1u, 8u, page_name(g_gui.active_page));
    draw_text(1u, 22u, "Focus:");
    draw_text(1u, 29u, focus_name(g_gui.focus_index));
    draw_text(1u, 44u, "Ticks:");
    uint64_to_decimal(current_ticks(), ticks_text, (uint32_t)sizeof(ticks_text));
    draw_text(1u, 51u, ticks_text);
    draw_text(2u, 2u, "Tab switch focus  Enter activate  Type text  Backspace delete");
    draw_button(3u, 2u, "Home", g_gui.focus_index == GUI_FOCUS_HOME);
    draw_button(3u, 12u, "Settings", g_gui.focus_index == GUI_FOCUS_SETTINGS);
    draw_button(3u, 27u, "Clear Input", g_gui.focus_index == GUI_FOCUS_CLEAR);
}

static void render_runtime_box(void) {
    char number_text[24];
    char scancode_text[8];

    draw_box(5u, 0u, g_gui.width, 7u, " Runtime ");

    uint64_to_decimal(g_gui.heartbeat_runs, number_text, (uint32_t)sizeof(number_text));
    draw_kv_line(6u, "Heartbeat", number_text);

    uint64_to_decimal(g_gui.key_events, number_text, (uint32_t)sizeof(number_text));
    draw_kv_line(7u, "Key Events", number_text);

    draw_kv_line(8u, "Last Key", g_gui.last_key);

    uint8_to_hex(g_gui.last_scancode, scancode_text, (uint32_t)sizeof(scancode_text));
    draw_kv_line(9u, "Scancode", scancode_text);

    draw_kv_line(10u, "Status", g_gui.status_message);
}

static void render_input_box(void) {
    char length_text[24];

    draw_box(12u, 0u, g_gui.width, 4u, " Demo Input ");
    draw_text(13u, 2u, (g_gui.input_length == 0u) ? "(type something)" : g_gui.input_buffer);
    uint64_to_decimal(g_gui.input_length, length_text, (uint32_t)sizeof(length_text));
    draw_text(14u, 2u, "Chars:");
    draw_text(14u, 9u, length_text);
    draw_text(14u, 12u, "/32");
}

static void render_home_page(uint32_t top, uint32_t height) {
    if (height < 4u) {
        return;
    }

    draw_box(top, 0u, g_gui.width, height, " Home Demo ");
    draw_text(top + 1u, 2u, "Desktop page is driven by the generic display/input interfaces.");
    draw_text(top + 2u, 2u, "[ Boot OK ] [ Keyboard OK ] [ Event Loop OK ]");
    draw_text(top + 3u, 2u, "Press Enter on >Home< to toggle extra detail cards.");

    if (g_gui.desktop_details && (height >= 7u)) {
        draw_text(top + 5u, 2u, "detail: timer ticks keep refreshing the runtime panel");
        draw_text(top + 6u, 2u, "detail: PS/2 key presses update focus, pages and input box");
    }
}

static void render_settings_page(uint32_t top, uint32_t height) {
    draw_box(top, 0u, g_gui.width, height, " Settings Demo ");
    draw_text(top + 1u, 2u, "Use >Settings< + Enter to toggle a live GUI flag.");
    draw_text(top + 2u, 2u, "Key echo:");
    draw_text(top + 2u, 12u, g_gui.settings_key_echo ? "enabled" : "disabled");
    draw_text(top + 3u, 2u, "Console logs stay on serial while the screen is owned by the GUI.");
}

void gui_init(void) {
    uint32_t width = 0u;
    uint32_t height = 0u;

    g_gui.ready = display_dimensions(&width, &height);
    if (!g_gui.ready) {
        return;
    }

    g_gui.width = width;
    g_gui.height = height;
    clear_screen();
}

void gui_note_heartbeat(void) {
    ++g_gui.heartbeat_runs;
}

void gui_handle_input_event(const struct input_event *event) {
    if ((event == NULL) || (event->type != INPUT_EVENT_KEY) || !event->pressed) {
        return;
    }

    ++g_gui.key_events;
    g_gui.last_scancode = event->scancode;
    set_last_key_label(event->character);

    if (event->character == '\t') {
        g_gui.focus_index = (uint8_t)((g_gui.focus_index + 1u) % GUI_FOCUS_COUNT);
        set_status_message("Focus moved to the next button.");
        return;
    }

    if (event->character == '\n') {
        activate_focus();
        return;
    }

    if (event->character == '\b') {
        erase_input_char();
        return;
    }

    if (is_printable_ascii(event->character)) {
        append_input_char(event->character);
        return;
    }

    set_status_message("Ignored a non-printable key.");
}

void gui_render(void) {
    uint32_t content_top = 16u;
    uint32_t content_height;

    if (!g_gui.ready) {
        return;
    }

    if ((g_gui.width < 40u) || (g_gui.height < 20u)) {
        clear_screen();
        draw_text(0u, 0u, "Display backend is too small for the GUI demo.");
        return;
    }

    clear_screen();
    render_header();
    render_runtime_box();
    render_input_box();

    if (g_gui.height <= content_top) {
        return;
    }

    content_height = g_gui.height - content_top;
    if (g_gui.active_page == GUI_PAGE_SETTINGS) {
        render_settings_page(content_top, content_height);
    } else {
        render_home_page(content_top, content_height);
    }
}
