#ifndef TINYOS_BOOT_INFO_H
#define TINYOS_BOOT_INFO_H

#include <stdbool.h>
#include <stdint.h>

enum tinyos_boot_method {
    TINYOS_BOOT_METHOD_UNKNOWN = 0,
    TINYOS_BOOT_METHOD_BIOS = 1,
    TINYOS_BOOT_METHOD_UEFI = 2
};

struct tinyos_framebuffer_info {
    uint64_t base;
    uint32_t width;
    uint32_t height;
    uint32_t pixels_per_scanline;
    uint32_t pixel_format;
    bool available;
};

struct tinyos_boot_info {
    uint32_t revision;
    uint32_t boot_method;
    uint64_t memory_map_address;
    uint64_t memory_map_size;
    uint64_t memory_map_descriptor_size;
    uint64_t rsdp_address;
    struct tinyos_framebuffer_info framebuffer;
    char boot_path[96];
};

#define TINYOS_BOOT_INFO_REVISION 1u

#endif
