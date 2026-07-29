#include "tinyos/gui_uefi.h"

#include <stddef.h>
#include <stdint.h>

#include "tinyos/arch.h"
#include "tinyos/lvgl_port.h"
#include "tinyos/memory.h"

#define UEFI_GUI_INPUT_CAPACITY 24u
#define UEFI_GUI_LAST_KEY_CAPACITY 16u
#define UEFI_GUI_STATUS_CAPACITY 40u

enum uefi_gui_focus_target {
    UEFI_GUI_FOCUS_HOME = 0,
    UEFI_GUI_FOCUS_SETTINGS = 1,
    UEFI_GUI_FOCUS_ABOUT = 2,
    UEFI_GUI_FOCUS_CLEAR = 3,
    UEFI_GUI_FOCUS_COUNT = 4
};

enum uefi_gui_page {
    UEFI_GUI_PAGE_HOME = 0,
    UEFI_GUI_PAGE_SETTINGS = 1,
    UEFI_GUI_PAGE_ABOUT = 2
};

struct glyph5x7 {
    char ch;
    uint8_t rows[7];
};

struct uefi_gui_state {
    bool ready;
    volatile uint32_t *framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t pixel_format;
    uint64_t heartbeat_runs;
    uint64_t key_events;
    uint64_t render_count;
    uint8_t focus_index;
    uint8_t active_page;
    bool settings_key_echo;
    uint8_t last_scancode;
    char last_key[UEFI_GUI_LAST_KEY_CAPACITY];
    char status_message[UEFI_GUI_STATUS_CAPACITY];
    char input_buffer[UEFI_GUI_INPUT_CAPACITY + 1u];
    uint32_t input_length;
};

static const struct glyph5x7 GLYPHS[] = {
    {' ', {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u}},
    {'-', {0x00u, 0x00u, 0x00u, 0x1Fu, 0x00u, 0x00u, 0x00u}},
    {'.', {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x0Cu, 0x0Cu}},
    {'/', {0x01u, 0x02u, 0x04u, 0x08u, 0x10u, 0x00u, 0x00u}},
    {'0', {0x0Eu, 0x11u, 0x13u, 0x15u, 0x19u, 0x11u, 0x0Eu}},
    {'1', {0x04u, 0x0Cu, 0x04u, 0x04u, 0x04u, 0x04u, 0x0Eu}},
    {'2', {0x0Eu, 0x11u, 0x01u, 0x02u, 0x04u, 0x08u, 0x1Fu}},
    {'3', {0x1Eu, 0x01u, 0x01u, 0x06u, 0x01u, 0x01u, 0x1Eu}},
    {'4', {0x02u, 0x06u, 0x0Au, 0x12u, 0x1Fu, 0x02u, 0x02u}},
    {'5', {0x1Fu, 0x10u, 0x10u, 0x1Eu, 0x01u, 0x01u, 0x1Eu}},
    {'6', {0x06u, 0x08u, 0x10u, 0x1Eu, 0x11u, 0x11u, 0x0Eu}},
    {'7', {0x1Fu, 0x01u, 0x02u, 0x04u, 0x08u, 0x08u, 0x08u}},
    {'8', {0x0Eu, 0x11u, 0x11u, 0x0Eu, 0x11u, 0x11u, 0x0Eu}},
    {'9', {0x0Eu, 0x11u, 0x11u, 0x0Fu, 0x01u, 0x02u, 0x0Cu}},
    {':', {0x00u, 0x0Cu, 0x0Cu, 0x00u, 0x0Cu, 0x0Cu, 0x00u}},
    {'A', {0x0Eu, 0x11u, 0x11u, 0x1Fu, 0x11u, 0x11u, 0x11u}},
    {'B', {0x1Eu, 0x11u, 0x11u, 0x1Eu, 0x11u, 0x11u, 0x1Eu}},
    {'C', {0x0Eu, 0x11u, 0x10u, 0x10u, 0x10u, 0x11u, 0x0Eu}},
    {'D', {0x1Cu, 0x12u, 0x11u, 0x11u, 0x11u, 0x12u, 0x1Cu}},
    {'E', {0x1Fu, 0x10u, 0x10u, 0x1Eu, 0x10u, 0x10u, 0x1Fu}},
    {'F', {0x1Fu, 0x10u, 0x10u, 0x1Eu, 0x10u, 0x10u, 0x10u}},
    {'G', {0x0Eu, 0x11u, 0x10u, 0x17u, 0x11u, 0x11u, 0x0Fu}},
    {'H', {0x11u, 0x11u, 0x11u, 0x1Fu, 0x11u, 0x11u, 0x11u}},
    {'I', {0x0Eu, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u, 0x0Eu}},
    {'J', {0x01u, 0x01u, 0x01u, 0x01u, 0x11u, 0x11u, 0x0Eu}},
    {'K', {0x11u, 0x12u, 0x14u, 0x18u, 0x14u, 0x12u, 0x11u}},
    {'L', {0x10u, 0x10u, 0x10u, 0x10u, 0x10u, 0x10u, 0x1Fu}},
    {'M', {0x11u, 0x1Bu, 0x15u, 0x15u, 0x11u, 0x11u, 0x11u}},
    {'N', {0x11u, 0x11u, 0x19u, 0x15u, 0x13u, 0x11u, 0x11u}},
    {'O', {0x0Eu, 0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x0Eu}},
    {'P', {0x1Eu, 0x11u, 0x11u, 0x1Eu, 0x10u, 0x10u, 0x10u}},
    {'Q', {0x0Eu, 0x11u, 0x11u, 0x11u, 0x15u, 0x12u, 0x0Du}},
    {'R', {0x1Eu, 0x11u, 0x11u, 0x1Eu, 0x14u, 0x12u, 0x11u}},
    {'S', {0x0Fu, 0x10u, 0x10u, 0x0Eu, 0x01u, 0x01u, 0x1Eu}},
    {'T', {0x1Fu, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u}},
    {'U', {0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x0Eu}},
    {'V', {0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x0Au, 0x04u}},
    {'W', {0x11u, 0x11u, 0x11u, 0x15u, 0x15u, 0x15u, 0x0Au}},
    {'X', {0x11u, 0x11u, 0x0Au, 0x04u, 0x0Au, 0x11u, 0x11u}},
    {'Y', {0x11u, 0x11u, 0x0Au, 0x04u, 0x04u, 0x04u, 0x04u}},
    {'Z', {0x1Fu, 0x01u, 0x02u, 0x04u, 0x08u, 0x10u, 0x1Fu}},
};

static struct uefi_gui_state g_gui = {
    .ready = false,
    .framebuffer = (volatile uint32_t *)0,
    .width = 0u,
    .height = 0u,
    .stride = 0u,
    .pixel_format = 0u,
    .heartbeat_runs = 0u,
    .key_events = 0u,
    .render_count = 0u,
    .focus_index = UEFI_GUI_FOCUS_HOME,
    .active_page = UEFI_GUI_PAGE_HOME,
    .settings_key_echo = true,
    .last_scancode = 0u,
    .last_key = "NONE",
    .status_message = "FRAMEBUFFER UI ACTIVE",
    .input_buffer = {0},
    .input_length = 0u
};
static bool g_gui_use_lvgl = false;

static uint64_t current_ticks(void) {
    const struct tinyos_arch_ops *arch = tinyos_arch_current();

    if ((arch == NULL) || (arch->timer_ticks == NULL)) {
        return 0u;
    }

    return arch->timer_ticks();
}

static uint32_t encode_color(uint32_t pixel_format, uint8_t red, uint8_t green, uint8_t blue) {
    if (pixel_format == 1u) {
        return ((uint32_t)blue) | ((uint32_t)green << 8u) | ((uint32_t)red << 16u);
    }

    return ((uint32_t)red) | ((uint32_t)green << 8u) | ((uint32_t)blue << 16u);
}

static void memory_zero(void *buffer, size_t size) {
    size_t index;
    uint8_t *bytes = (uint8_t *)buffer;

    for (index = 0u; index < size; ++index) {
        bytes[index] = 0u;
    }
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

static char upper_ascii(char ch) {
    if ((ch >= 'a') && (ch <= 'z')) {
        return (char)(ch - ('a' - 'A'));
    }

    return ch;
}

static bool is_printable_ascii(char ch) {
    return (ch >= 32) && (ch <= 126);
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
    buffer[1] = 'X';
    buffer[2] = digits[(value >> 4u) & 0x0Fu];
    buffer[3] = digits[value & 0x0Fu];
    buffer[4] = '\0';
}

static void format_resolution(char *buffer, uint32_t capacity) {
    char width_text[16];
    char height_text[16];
    uint32_t index = 0u;
    uint32_t part = 0u;

    if ((buffer == NULL) || (capacity == 0u)) {
        return;
    }

    uint64_to_decimal((uint64_t)g_gui.width, width_text, (uint32_t)sizeof(width_text));
    uint64_to_decimal((uint64_t)g_gui.height, height_text, (uint32_t)sizeof(height_text));

    while ((width_text[part] != '\0') && (index + 1u < capacity)) {
        buffer[index++] = width_text[part++];
    }

    if (index + 1u < capacity) {
        buffer[index++] = 'X';
    }

    part = 0u;
    while ((height_text[part] != '\0') && (index + 1u < capacity)) {
        buffer[index++] = height_text[part++];
    }
    buffer[index] = '\0';
}

static void set_status_message(const char *message) {
    copy_string(g_gui.status_message, UEFI_GUI_STATUS_CAPACITY, message);
}

static const char *page_name(uint8_t page) {
    if (page == UEFI_GUI_PAGE_SETTINGS) {
        return "SETTINGS";
    }

    if (page == UEFI_GUI_PAGE_ABOUT) {
        return "ABOUT";
    }

    return "HOME";
}

static const char *pixel_format_name(uint32_t pixel_format) {
    if (pixel_format == 1u) {
        return "BGR";
    }

    return "RGB";
}

static void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if ((x >= g_gui.width) || (y >= g_gui.height) || (g_gui.framebuffer == (volatile uint32_t *)0)) {
        return;
    }

    g_gui.framebuffer[(size_t)y * (size_t)g_gui.stride + (size_t)x] = color;
}

static void fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    uint32_t row;
    uint32_t col;

    for (row = 0u; row < height; ++row) {
        for (col = 0u; col < width; ++col) {
            put_pixel(x + col, y + row, color);
        }
    }
}

static void stroke_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    uint32_t index;

    if ((width < 2u) || (height < 2u)) {
        return;
    }

    for (index = 0u; index < width; ++index) {
        put_pixel(x + index, y, color);
        put_pixel(x + index, y + height - 1u, color);
    }

    for (index = 0u; index < height; ++index) {
        put_pixel(x, y + index, color);
        put_pixel(x + width - 1u, y + index, color);
    }
}

static const struct glyph5x7 *find_glyph(char ch) {
    size_t index;

    ch = upper_ascii(ch);
    for (index = 0u; index < sizeof(GLYPHS) / sizeof(GLYPHS[0]); ++index) {
        if (GLYPHS[index].ch == ch) {
            return &GLYPHS[index];
        }
    }

    return &GLYPHS[0];
}

static void draw_text(uint32_t x, uint32_t y, uint32_t scale, uint32_t color, const char *text) {
    uint32_t cursor_x = x;

    while ((text != NULL) && (*text != '\0')) {
        const struct glyph5x7 *glyph = find_glyph(*text);
        uint32_t row;

        for (row = 0u; row < 7u; ++row) {
            uint32_t col;

            for (col = 0u; col < 5u; ++col) {
                if ((glyph->rows[row] & (uint8_t)(1u << (4u - col))) != 0u) {
                    fill_rect(cursor_x + col * scale, y + row * scale, scale, scale, color);
                }
            }
        }

        cursor_x += 6u * scale;
        ++text;
    }
}

static void draw_label_value(
    uint32_t x,
    uint32_t y,
    uint32_t scale,
    uint32_t label_color,
    uint32_t value_color,
    const char *label,
    const char *value
) {
    uint32_t value_x = x + (string_length(label) * 6u + 2u) * scale;

    draw_text(x, y, scale, label_color, label);
    draw_text(value_x, y, scale, value_color, value);
}

static void set_last_key_label(char ch) {
    if (ch == '\n') {
        copy_string(g_gui.last_key, UEFI_GUI_LAST_KEY_CAPACITY, "ENTER");
        return;
    }

    if (ch == '\t') {
        copy_string(g_gui.last_key, UEFI_GUI_LAST_KEY_CAPACITY, "TAB");
        return;
    }

    if (ch == '\b') {
        copy_string(g_gui.last_key, UEFI_GUI_LAST_KEY_CAPACITY, "BACK");
        return;
    }

    if (ch == ' ') {
        copy_string(g_gui.last_key, UEFI_GUI_LAST_KEY_CAPACITY, "SPACE");
        return;
    }

    if (is_printable_ascii(ch)) {
        g_gui.last_key[0] = upper_ascii(ch);
        g_gui.last_key[1] = '\0';
        return;
    }

    copy_string(g_gui.last_key, UEFI_GUI_LAST_KEY_CAPACITY, "OTHER");
}

static void append_input_char(char ch) {
    if (g_gui.input_length >= UEFI_GUI_INPUT_CAPACITY) {
        set_status_message("INPUT BUFFER FULL");
        return;
    }

    g_gui.input_buffer[g_gui.input_length] = upper_ascii(ch);
    ++g_gui.input_length;
    g_gui.input_buffer[g_gui.input_length] = '\0';

    if (g_gui.settings_key_echo) {
        set_status_message("INPUT UPDATED");
    }
}

static void erase_input_char(void) {
    if (g_gui.input_length == 0u) {
        set_status_message("INPUT ALREADY EMPTY");
        return;
    }

    --g_gui.input_length;
    g_gui.input_buffer[g_gui.input_length] = '\0';
    set_status_message("INPUT ERASED");
}

static void activate_focus(void) {
    if (g_gui.focus_index == UEFI_GUI_FOCUS_HOME) {
        g_gui.active_page = UEFI_GUI_PAGE_HOME;
        set_status_message("HOME PAGE ACTIVE");
        return;
    }

    if (g_gui.focus_index == UEFI_GUI_FOCUS_SETTINGS) {
        g_gui.active_page = UEFI_GUI_PAGE_SETTINGS;
        g_gui.settings_key_echo = !g_gui.settings_key_echo;
        set_status_message(g_gui.settings_key_echo ? "KEY ECHO ENABLED" : "KEY ECHO DISABLED");
        return;
    }

    if (g_gui.focus_index == UEFI_GUI_FOCUS_ABOUT) {
        g_gui.active_page = UEFI_GUI_PAGE_ABOUT;
        set_status_message("ABOUT PAGE ACTIVE");
        return;
    }

    g_gui.input_length = 0u;
    g_gui.input_buffer[0] = '\0';
    set_status_message("INPUT CLEARED");
}

static void render_button(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    const char *label,
    bool focused,
    uint32_t fill_color,
    uint32_t accent_color,
    uint32_t text_color
) {
    uint32_t border = focused ? accent_color : encode_color(g_gui.pixel_format, 70u, 100u, 140u);

    fill_rect(x, y, width, height, fill_color);
    stroke_rect(x, y, width, height, border);
    draw_text(x + 14u, y + 10u, 3u, text_color, label);
}

static void render_home_page(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t text_color, uint32_t accent_color) {
    char resolution[24];
    char heap_text[24];

    (void)width;
    (void)height;
    format_resolution(resolution, (uint32_t)sizeof(resolution));
    uint64_to_decimal((uint64_t)memory_bytes_used(), heap_text, (uint32_t)sizeof(heap_text));

    draw_text(x + 18u, y + 18u, 4u, accent_color, "DASHBOARD");
    draw_text(x + 18u, y + 64u, 2u, text_color, "UEFI FRAMEBUFFER ACTIVE");
    draw_text(x + 18u, y + 90u, 2u, text_color, "EVENT LOOP ACTIVE");
    draw_text(x + 18u, y + 116u, 2u, text_color, "TAB NEXT  ENTER OPEN");
    draw_text(x + 18u, y + 142u, 2u, text_color, "123 DIRECT PAGE KEYS");
    draw_label_value(x + 18u, y + 182u, 2u, accent_color, text_color, "DISPLAY", resolution);
    draw_label_value(x + 18u, y + 208u, 2u, accent_color, text_color, "BOOT", "UEFI");
    draw_label_value(x + 18u, y + 234u, 2u, accent_color, text_color, "HEAP", heap_text);
}

static void render_settings_page(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t text_color, uint32_t accent_color) {
    char render_text[24];

    (void)width;
    (void)height;
    uint64_to_decimal(g_gui.render_count, render_text, (uint32_t)sizeof(render_text));

    draw_text(x + 18u, y + 18u, 4u, accent_color, "SETTINGS");
    draw_label_value(x + 18u, y + 72u, 2u, accent_color, text_color, "KEY ECHO", g_gui.settings_key_echo ? "ON" : "OFF");
    draw_label_value(x + 18u, y + 98u, 2u, accent_color, text_color, "PIXEL", pixel_format_name(g_gui.pixel_format));
    draw_label_value(x + 18u, y + 124u, 2u, accent_color, text_color, "RENDER", render_text);
    draw_text(x + 18u, y + 164u, 2u, text_color, "OPEN SETTINGS TO TOGGLE");
    draw_text(x + 18u, y + 190u, 2u, text_color, "KEY ECHO FOR INPUT FIELD");
}

static void render_about_page(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t text_color, uint32_t accent_color) {
    (void)width;
    (void)height;

    draw_text(x + 18u, y + 18u, 4u, accent_color, "ABOUT");
    draw_text(x + 18u, y + 72u, 2u, text_color, "GUI CORE READY FOR LVGL");
    draw_text(x + 18u, y + 98u, 2u, text_color, "NEXT STEP IS DRIVER PORT");
    draw_text(x + 18u, y + 124u, 2u, text_color, "DISPLAY FLUSH INPUT TICK");
    draw_text(x + 18u, y + 150u, 2u, text_color, "CURRENT UI IS BUILTIN");
    draw_text(x + 18u, y + 176u, 2u, text_color, "FOR FAST UEFI ITERATION");
    draw_label_value(x + 18u, y + 216u, 2u, accent_color, text_color, "PAGE", page_name(g_gui.active_page));
}

bool tinyos_uefi_gui_supported(const struct tinyos_boot_info *boot_info) {
    if ((boot_info == (const struct tinyos_boot_info *)0) ||
        (boot_info->boot_method != TINYOS_BOOT_METHOD_UEFI) ||
        !boot_info->framebuffer.available || (boot_info->framebuffer.base == 0u)) {
        return false;
    }

    return (boot_info->framebuffer.width >= 640u) &&
           (boot_info->framebuffer.height >= 360u) &&
           (boot_info->framebuffer.pixels_per_scanline >= boot_info->framebuffer.width);
}

void tinyos_uefi_gui_init(const struct tinyos_boot_info *boot_info) {
    memory_zero(&g_gui, sizeof(g_gui));
    g_gui.ready = tinyos_uefi_gui_supported(boot_info);
    g_gui_use_lvgl = false;
    if (!g_gui.ready) {
        return;
    }

    if (tinyos_lvgl_init(boot_info)) {
        g_gui_use_lvgl = true;
        return;
    }

    g_gui.framebuffer = (volatile uint32_t *)(uintptr_t)boot_info->framebuffer.base;
    g_gui.width = boot_info->framebuffer.width;
    g_gui.height = boot_info->framebuffer.height;
    g_gui.stride = boot_info->framebuffer.pixels_per_scanline;
    g_gui.pixel_format = boot_info->framebuffer.pixel_format;
    g_gui.focus_index = UEFI_GUI_FOCUS_HOME;
    g_gui.active_page = UEFI_GUI_PAGE_HOME;
    g_gui.settings_key_echo = true;
    copy_string(g_gui.last_key, UEFI_GUI_LAST_KEY_CAPACITY, "NONE");
    copy_string(g_gui.status_message, UEFI_GUI_STATUS_CAPACITY, "FRAMEBUFFER UI ACTIVE");
}

void tinyos_uefi_gui_note_heartbeat(void) {
    if (g_gui_use_lvgl) {
        tinyos_lvgl_note_heartbeat();
        return;
    }

    ++g_gui.heartbeat_runs;
}

void tinyos_uefi_gui_handle_input_event(const struct input_event *event) {
    if (g_gui_use_lvgl) {
        tinyos_lvgl_handle_input_event(event);
        return;
    }

    if (!g_gui.ready || (event == (const struct input_event *)0) || (event->type != INPUT_EVENT_KEY) || !event->pressed) {
        return;
    }

    ++g_gui.key_events;
    g_gui.last_scancode = event->scancode;
    set_last_key_label(event->character);

    if (event->character == '1') {
        g_gui.focus_index = UEFI_GUI_FOCUS_HOME;
        g_gui.active_page = UEFI_GUI_PAGE_HOME;
        set_status_message("HOME PAGE ACTIVE");
        return;
    }

    if (event->character == '2') {
        g_gui.focus_index = UEFI_GUI_FOCUS_SETTINGS;
        g_gui.active_page = UEFI_GUI_PAGE_SETTINGS;
        set_status_message("SETTINGS PAGE ACTIVE");
        return;
    }

    if (event->character == '3') {
        g_gui.focus_index = UEFI_GUI_FOCUS_ABOUT;
        g_gui.active_page = UEFI_GUI_PAGE_ABOUT;
        set_status_message("ABOUT PAGE ACTIVE");
        return;
    }

    if ((event->character == '0') || (event->character == 'c') || (event->character == 'C')) {
        g_gui.focus_index = UEFI_GUI_FOCUS_CLEAR;
        g_gui.input_length = 0u;
        g_gui.input_buffer[0] = '\0';
        set_status_message("INPUT CLEARED");
        return;
    }

    if (event->character == '\t') {
        g_gui.focus_index = (uint8_t)((g_gui.focus_index + 1u) % UEFI_GUI_FOCUS_COUNT);
        set_status_message("FOCUS MOVED");
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

    set_status_message("IGNORED KEY");
}

void tinyos_uefi_gui_render(void) {
    uint32_t background;
    uint32_t panel;
    uint32_t card;
    uint32_t accent;
    uint32_t text;
    uint32_t subdued;
    uint32_t top_band_height;
    uint32_t header_y;
    uint32_t left_panel_x;
    uint32_t left_panel_y;
    uint32_t left_panel_w;
    uint32_t left_panel_h;
    uint32_t right_panel_x;
    uint32_t right_panel_w;
    uint32_t right_panel_h;
    uint32_t input_panel_y;
    uint32_t input_panel_h;
    uint32_t button_y;
    uint32_t button_w;
    uint32_t stats_y;
    uint32_t footer_y;
    uint32_t meter_x;
    uint32_t meter_y;
    uint32_t meter_w;
    uint32_t meter_fill;
    char number_text[24];
    char hex_text[8];

    if (g_gui_use_lvgl) {
        tinyos_lvgl_render();
        return;
    }

    if (!g_gui.ready) {
        return;
    }

    ++g_gui.render_count;

    background = encode_color(g_gui.pixel_format, 6u, 14u, 32u);
    panel = encode_color(g_gui.pixel_format, 24u, 42u, 72u);
    card = encode_color(g_gui.pixel_format, 15u, 28u, 54u);
    accent = encode_color(g_gui.pixel_format, 58u, 170u, 240u);
    text = encode_color(g_gui.pixel_format, 232u, 241u, 255u);
    subdued = encode_color(g_gui.pixel_format, 154u, 180u, 214u);

    fill_rect(0u, 0u, g_gui.width, g_gui.height, background);

    top_band_height = g_gui.height / 9u;
    fill_rect(0u, 0u, g_gui.width, top_band_height, accent);
    draw_text(24u, 20u, 5u, text, "TINYOS");
    draw_text(g_gui.width - 240u, 24u, 3u, text, "UEFI UI");
    draw_label_value(g_gui.width - 360u, top_band_height - 34u, 2u, text, text, "PAGE", page_name(g_gui.active_page));

    header_y = top_band_height + 16u;
    button_y = header_y;
    button_w = (g_gui.width - 96u) / 4u;
    render_button(20u, button_y, button_w - 8u, 48u, "HOME", g_gui.focus_index == UEFI_GUI_FOCUS_HOME, panel, accent, text);
    render_button(28u + button_w, button_y, button_w - 8u, 48u, "SETTINGS", g_gui.focus_index == UEFI_GUI_FOCUS_SETTINGS, panel, accent, text);
    render_button(36u + button_w * 2u, button_y, button_w - 8u, 48u, "ABOUT", g_gui.focus_index == UEFI_GUI_FOCUS_ABOUT, panel, accent, text);
    render_button(44u + button_w * 3u, button_y, button_w - 8u, 48u, "CLEAR", g_gui.focus_index == UEFI_GUI_FOCUS_CLEAR, panel, accent, text);

    left_panel_x = 20u;
    left_panel_y = button_y + 68u;
    input_panel_h = g_gui.height / 5u;
    input_panel_y = g_gui.height - input_panel_h - 20u;
    left_panel_w = g_gui.width / 3u;
    left_panel_h = input_panel_y - left_panel_y - 12u;
    right_panel_x = left_panel_x + left_panel_w + 16u;
    right_panel_w = g_gui.width - right_panel_x - 20u;
    right_panel_h = left_panel_h;

    fill_rect(left_panel_x, left_panel_y, left_panel_w, left_panel_h, panel);
    stroke_rect(left_panel_x, left_panel_y, left_panel_w, left_panel_h, accent);
    draw_text(left_panel_x + 18u, left_panel_y + 16u, 4u, accent, "RUNTIME");

    stats_y = left_panel_y + 64u;
    uint64_to_decimal(current_ticks(), number_text, (uint32_t)sizeof(number_text));
    draw_label_value(left_panel_x + 18u, stats_y, 2u, accent, text, "TICKS", number_text);
    uint64_to_decimal(g_gui.heartbeat_runs, number_text, (uint32_t)sizeof(number_text));
    draw_label_value(left_panel_x + 18u, stats_y + 28u, 2u, accent, text, "HEART", number_text);
    uint64_to_decimal(g_gui.key_events, number_text, (uint32_t)sizeof(number_text));
    draw_label_value(left_panel_x + 18u, stats_y + 56u, 2u, accent, text, "EVENTS", number_text);
    draw_label_value(left_panel_x + 18u, stats_y + 84u, 2u, accent, text, "LAST", g_gui.last_key);
    uint8_to_hex(g_gui.last_scancode, hex_text, (uint32_t)sizeof(hex_text));
    draw_label_value(left_panel_x + 18u, stats_y + 112u, 2u, accent, text, "SCAN", hex_text);
    draw_label_value(left_panel_x + 18u, stats_y + 140u, 2u, accent, subdued, "STATUS", g_gui.status_message);
    draw_label_value(left_panel_x + 18u, stats_y + 168u, 2u, accent, text, "PAGE", page_name(g_gui.active_page));

    fill_rect(right_panel_x, left_panel_y, right_panel_w, right_panel_h, card);
    stroke_rect(right_panel_x, left_panel_y, right_panel_w, right_panel_h, accent);
    if (g_gui.active_page == UEFI_GUI_PAGE_SETTINGS) {
        render_settings_page(right_panel_x, left_panel_y, right_panel_w, right_panel_h, text, accent);
    } else if (g_gui.active_page == UEFI_GUI_PAGE_ABOUT) {
        render_about_page(right_panel_x, left_panel_y, right_panel_w, right_panel_h, text, accent);
    } else {
        render_home_page(right_panel_x, left_panel_y, right_panel_w, right_panel_h, text, accent);
    }

    fill_rect(left_panel_x, input_panel_y, g_gui.width - 40u, input_panel_h, panel);
    stroke_rect(left_panel_x, input_panel_y, g_gui.width - 40u, input_panel_h, accent);
    draw_text(left_panel_x + 18u, input_panel_y + 16u, 4u, accent, "INPUT");
    draw_text(left_panel_x + 18u, input_panel_y + 64u, 3u, text, (g_gui.input_length == 0u) ? "TYPE ON KEYBOARD" : g_gui.input_buffer);

    footer_y = g_gui.height - 28u;
    draw_text(20u, footer_y, 2u, subdued, "TAB NEXT  ENTER OPEN  123 PAGES  0 CLEAR");
    meter_x = g_gui.width - 280u;
    meter_y = footer_y - 2u;
    meter_w = 220u;
    fill_rect(meter_x, meter_y, meter_w, 14u, card);
    stroke_rect(meter_x, meter_y, meter_w, 14u, accent);
    meter_fill = (uint32_t)((current_ticks() + g_gui.heartbeat_runs) % (uint64_t)meter_w);
    if (meter_fill > 0u) {
        fill_rect(meter_x + 1u, meter_y + 1u, meter_fill, 12u, accent);
    }
}
