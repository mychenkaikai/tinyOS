#include "tinyos/uefi_boot_demo.h"

#include <stddef.h>
#include <stdint.h>

struct glyph8 {
    char ch;
    uint8_t rows[8];
};

static const struct glyph8 GLYPHS[] = {
    {' ', {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u}},
    {'B', {0x7Cu, 0x42u, 0x42u, 0x7Cu, 0x42u, 0x42u, 0x7Cu, 0x00u}},
    {'E', {0x7Eu, 0x40u, 0x40u, 0x7Cu, 0x40u, 0x40u, 0x7Eu, 0x00u}},
    {'F', {0x7Eu, 0x40u, 0x40u, 0x7Cu, 0x40u, 0x40u, 0x40u, 0x00u}},
    {'I', {0x7Eu, 0x18u, 0x18u, 0x18u, 0x18u, 0x18u, 0x7Eu, 0x00u}},
    {'N', {0x42u, 0x62u, 0x52u, 0x4Au, 0x46u, 0x42u, 0x42u, 0x00u}},
    {'O', {0x3Cu, 0x42u, 0x42u, 0x42u, 0x42u, 0x42u, 0x3Cu, 0x00u}},
    {'S', {0x3Eu, 0x40u, 0x40u, 0x3Cu, 0x02u, 0x02u, 0x7Cu, 0x00u}},
    {'T', {0x7Eu, 0x18u, 0x18u, 0x18u, 0x18u, 0x18u, 0x18u, 0x00u}},
    {'U', {0x42u, 0x42u, 0x42u, 0x42u, 0x42u, 0x42u, 0x3Cu, 0x00u}},
    {'Y', {0x42u, 0x42u, 0x24u, 0x18u, 0x18u, 0x18u, 0x18u, 0x00u}},
};

static uint32_t encode_color(uint32_t pixel_format, uint8_t red, uint8_t green, uint8_t blue) {
    if (pixel_format == 1u) {
        return ((uint32_t)blue) | ((uint32_t)green << 8u) | ((uint32_t)red << 16u);
    }

    return ((uint32_t)red) | ((uint32_t)green << 8u) | ((uint32_t)blue << 16u);
}

static void put_pixel(
    volatile uint32_t *framebuffer,
    uint32_t pixels_per_scanline,
    uint32_t width,
    uint32_t height,
    uint32_t x,
    uint32_t y,
    uint32_t color
) {
    if ((x >= width) || (y >= height)) {
        return;
    }

    framebuffer[(size_t)y * (size_t)pixels_per_scanline + (size_t)x] = color;
}

static void fill_rect(
    volatile uint32_t *framebuffer,
    uint32_t pixels_per_scanline,
    uint32_t width,
    uint32_t height,
    uint32_t x,
    uint32_t y,
    uint32_t rect_width,
    uint32_t rect_height,
    uint32_t color
) {
    uint32_t row;
    uint32_t col;

    for (row = 0u; row < rect_height; ++row) {
        for (col = 0u; col < rect_width; ++col) {
            put_pixel(framebuffer, pixels_per_scanline, width, height, x + col, y + row, color);
        }
    }
}

static const struct glyph8 *find_glyph(char ch) {
    size_t index;

    for (index = 0u; index < sizeof(GLYPHS) / sizeof(GLYPHS[0]); ++index) {
        if (GLYPHS[index].ch == ch) {
            return &GLYPHS[index];
        }
    }

    return &GLYPHS[0];
}

static void draw_text(
    volatile uint32_t *framebuffer,
    uint32_t pixels_per_scanline,
    uint32_t width,
    uint32_t height,
    uint32_t x,
    uint32_t y,
    uint32_t scale,
    uint32_t color,
    const char *text
) {
    uint32_t cursor_x = x;

    while ((text != NULL) && (*text != '\0')) {
        const struct glyph8 *glyph = find_glyph(*text);
        uint32_t row;
        for (row = 0u; row < 8u; ++row) {
            uint32_t col;
            for (col = 0u; col < 8u; ++col) {
                if ((glyph->rows[row] & (uint8_t)(0x80u >> col)) != 0u) {
                    fill_rect(
                        framebuffer,
                        pixels_per_scanline,
                        width,
                        height,
                        cursor_x + col * scale,
                        y + row * scale,
                        scale,
                        scale,
                        color
                    );
                }
            }
        }
        cursor_x += 9u * scale;
        ++text;
    }
}

bool tinyos_uefi_boot_demo_run(const struct tinyos_boot_info *boot_info) {
    volatile uint32_t *framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t background;
    uint32_t panel;
    uint32_t accent;
    uint32_t text;
    uint32_t band_height;

    if ((boot_info == NULL) || (boot_info->boot_method != TINYOS_BOOT_METHOD_UEFI) ||
        !boot_info->framebuffer.available || (boot_info->framebuffer.base == 0u)) {
        return false;
    }

    framebuffer = (volatile uint32_t *)(uintptr_t)boot_info->framebuffer.base;
    width = boot_info->framebuffer.width;
    height = boot_info->framebuffer.height;
    stride = boot_info->framebuffer.pixels_per_scanline;
    if ((framebuffer == NULL) || (width < 320u) || (height < 200u) || (stride < width)) {
        return false;
    }

    background = encode_color(boot_info->framebuffer.pixel_format, 12u, 18u, 34u);
    panel = encode_color(boot_info->framebuffer.pixel_format, 28u, 52u, 86u);
    accent = encode_color(boot_info->framebuffer.pixel_format, 54u, 162u, 235u);
    text = encode_color(boot_info->framebuffer.pixel_format, 235u, 244u, 255u);

    fill_rect(framebuffer, stride, width, height, 0u, 0u, width, height, background);
    band_height = height / 8u;
    fill_rect(framebuffer, stride, width, height, 0u, 0u, width, band_height, accent);
    fill_rect(framebuffer, stride, width, height, width / 10u, height / 4u, width * 8u / 10u, height / 2u, panel);
    fill_rect(framebuffer, stride, width, height, width / 10u, height / 4u, width * 8u / 10u, 4u, accent);
    fill_rect(framebuffer, stride, width, height, width / 10u, height * 3u / 4u - 4u, width * 8u / 10u, 4u, accent);

    draw_text(framebuffer, stride, width, height, width / 6u, height / 3u, 4u, text, "TINYOS");
    draw_text(framebuffer, stride, width, height, width / 6u, height / 2u, 3u, text, "UEFI BOOT");
    return true;
}
