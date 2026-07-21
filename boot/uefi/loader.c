#include "efi/efi.h"
#include "efi/efilib.h"
#include "tinyos/boot_info.h"

#define TINYOS_KERNEL_LOAD_ADDRESS 0x02000000ULL
static struct tinyos_boot_info g_boot_info;

// #region debug-point C:loader-debugcon
static void debugcon_write(UINT8 marker) {
    __asm__ volatile ("outb %0, %1" : : "a"(marker), "Nd"((UINT16)0x402));
}
// #endregion

static void memory_zero(void *destination, UINTN size) {
    UINT8 *dst = (UINT8 *)destination;
    UINTN index;

    for (index = 0u; index < size; ++index) {
        dst[index] = 0u;
    }
}

static void copy_ascii(char *destination, UINTN capacity, const char *source) {
    UINTN index = 0u;

    if ((destination == (char *)0) || (capacity == 0u)) {
        return;
    }

    while ((source != (const char *)0) && (source[index] != '\0') && (index + 1u < capacity)) {
        destination[index] = source[index];
        ++index;
    }

    destination[index] = '\0';
}

static void write_ascii(EFI_SYSTEM_TABLE *system_table, const char *message) {
    CHAR16 buffer[128];
    UINTN source_index = 0u;
    UINTN output_index = 0u;

    if ((system_table == (EFI_SYSTEM_TABLE *)0) || (system_table->ConOut == (EFI_SIMPLE_TEXT_OUT_PROTOCOL *)0)) {
        return;
    }

    while ((message != (const char *)0) && (message[source_index] != '\0') &&
           (output_index + 3u < (UINTN)(sizeof(buffer) / sizeof(buffer[0])))) {
        if (message[source_index] == '\n') {
            buffer[output_index++] = (CHAR16)'\r';
            buffer[output_index++] = (CHAR16)'\n';
            ++source_index;
            continue;
        }

        buffer[output_index++] = (CHAR16)(UINT8)message[source_index++];
    }

    buffer[output_index] = 0;
    uefi_call_wrapper(system_table->ConOut->OutputString, 2, system_table->ConOut, buffer);
}

static int guid_equal(const EFI_GUID *left, const EFI_GUID *right) {
    return (left->Data1 == right->Data1) &&
           (left->Data2 == right->Data2) &&
           (left->Data3 == right->Data3) &&
           (left->Data4[0] == right->Data4[0]) &&
           (left->Data4[1] == right->Data4[1]) &&
           (left->Data4[2] == right->Data4[2]) &&
           (left->Data4[3] == right->Data4[3]) &&
           (left->Data4[4] == right->Data4[4]) &&
           (left->Data4[5] == right->Data4[5]) &&
           (left->Data4[6] == right->Data4[6]) &&
           (left->Data4[7] == right->Data4[7]);
}

static void capture_acpi_rsdp(EFI_SYSTEM_TABLE *system_table) {
    UINTN index;
    EFI_GUID acpi20_table_guid = ACPI_20_TABLE_GUID;

    g_boot_info.rsdp_address = 0u;
    if (system_table == (EFI_SYSTEM_TABLE *)0) {
        return;
    }

    for (index = 0u; index < system_table->NumberOfTableEntries; ++index) {
        EFI_CONFIGURATION_TABLE *entry = &system_table->ConfigurationTable[index];
        if (guid_equal(&entry->VendorGuid, &acpi20_table_guid)) {
            g_boot_info.rsdp_address = (UINT64)(UINTN)entry->VendorTable;
            return;
        }
    }
}

static void capture_framebuffer(EFI_SYSTEM_TABLE *system_table) {
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = (EFI_GRAPHICS_OUTPUT_PROTOCOL *)0;

    g_boot_info.framebuffer.available = false;
    if ((system_table == (EFI_SYSTEM_TABLE *)0) || (system_table->BootServices == (EFI_BOOT_SERVICES *)0)) {
        return;
    }

    status = uefi_call_wrapper(system_table->BootServices->LocateProtocol, 3, &GraphicsOutputProtocol, (void *)0, (void **)&gop);
    if (EFI_ERROR(status) || (gop == (EFI_GRAPHICS_OUTPUT_PROTOCOL *)0) || (gop->Mode == (EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *)0) ||
        (gop->Mode->Info == (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *)0)) {
        return;
    }

    g_boot_info.framebuffer.available = true;
    g_boot_info.framebuffer.base = gop->Mode->FrameBufferBase;
    g_boot_info.framebuffer.width = gop->Mode->Info->HorizontalResolution;
    g_boot_info.framebuffer.height = gop->Mode->Info->VerticalResolution;
    g_boot_info.framebuffer.pixels_per_scanline = gop->Mode->Info->PixelsPerScanLine;
    g_boot_info.framebuffer.pixel_format = (UINT32)gop->Mode->Info->PixelFormat;
}

static EFI_STATUS load_kernel_image(EFI_HANDLE image_handle, EFI_PHYSICAL_ADDRESS *kernel_address, UINT64 *kernel_size) {
    EFI_STATUS status;
    EFI_LOADED_IMAGE *loaded_image = NULL;
    EFI_FILE_HANDLE root = NULL;
    EFI_FILE_HANDLE kernel_file = NULL;
    EFI_FILE_INFO *file_info = NULL;
    UINTN page_count;
    UINTN read_size;
    EFI_PHYSICAL_ADDRESS target_address = TINYOS_KERNEL_LOAD_ADDRESS;
    UINT64 file_size;
    static CHAR16 *const kernel_paths[] = {
        L"\\KERNEL.BIN",
        L"\\EFI\\BOOT\\KERNEL.BIN",
    };
    UINTN path_index;

    if ((kernel_address == NULL) || (kernel_size == NULL)) {
        return EFI_INVALID_PARAMETER;
    }

    status = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle, &LoadedImageProtocol, (void **)&loaded_image);
    if (EFI_ERROR(status) || (loaded_image == NULL)) {
        return EFI_NOT_FOUND;
    }

    root = LibOpenRoot(loaded_image->DeviceHandle);
    if (root == NULL) {
        return EFI_NOT_FOUND;
    }

    for (path_index = 0u; path_index < (UINTN)(sizeof(kernel_paths) / sizeof(kernel_paths[0])); ++path_index) {
        status = uefi_call_wrapper(root->Open, 5, root, &kernel_file, kernel_paths[path_index], EFI_FILE_MODE_READ, 0u);
        if (!EFI_ERROR(status) && (kernel_file != NULL)) {
            break;
        }
    }

    if (kernel_file == NULL) {
        uefi_call_wrapper(root->Close, 1, root);
        return EFI_NOT_FOUND;
    }

    file_info = LibFileInfo(kernel_file);
    if ((file_info == NULL) || (file_info->FileSize == 0u)) {
        uefi_call_wrapper(kernel_file->Close, 1, kernel_file);
        uefi_call_wrapper(root->Close, 1, root);
        return EFI_LOAD_ERROR;
    }

    file_size = file_info->FileSize;
    page_count = (UINTN)((file_size + 4095u) / 4096u);
    status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAddress, EfiLoaderCode, page_count, &target_address);
    if (EFI_ERROR(status)) {
        uefi_call_wrapper(BS->FreePool, 1, file_info);
        uefi_call_wrapper(kernel_file->Close, 1, kernel_file);
        uefi_call_wrapper(root->Close, 1, root);
        return EFI_OUT_OF_RESOURCES;
    }

    read_size = (UINTN)file_size;
    status = uefi_call_wrapper(kernel_file->Read, 3, kernel_file, &read_size, (void *)(UINTN)target_address);
    uefi_call_wrapper(BS->FreePool, 1, file_info);
    uefi_call_wrapper(kernel_file->Close, 1, kernel_file);
    uefi_call_wrapper(root->Close, 1, root);
    if (EFI_ERROR(status) || (read_size != (UINTN)file_size)) {
        return EFI_LOAD_ERROR;
    }

    *kernel_address = target_address;
    *kernel_size = file_size;
    return EFI_SUCCESS;
}

static EFI_STATUS capture_memory_map(UINTN *map_key) {
    EFI_STATUS status;
    UINTN descriptor_size = 0u;
    UINT32 descriptor_version = 0u;
    UINTN memory_map_size = 0u;
    void *memory_map = NULL;

    if (map_key == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    status = uefi_call_wrapper(BS->GetMemoryMap, 5, &memory_map_size, (EFI_MEMORY_DESCRIPTOR *)0, map_key, &descriptor_size, &descriptor_version);
    if (status != EFI_BUFFER_TOO_SMALL) {
        return EFI_ABORTED;
    }

    memory_map_size += descriptor_size * 8u;
    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, memory_map_size, &memory_map);
    if (EFI_ERROR(status)) {
        return EFI_OUT_OF_RESOURCES;
    }

    status = uefi_call_wrapper(BS->GetMemoryMap, 5, &memory_map_size, (EFI_MEMORY_DESCRIPTOR *)memory_map, map_key, &descriptor_size, &descriptor_version);
    if (EFI_ERROR(status)) {
        uefi_call_wrapper(BS->FreePool, 1, memory_map);
        return EFI_ABORTED;
    }

    g_boot_info.memory_map_address = (UINT64)(UINTN)memory_map;
    g_boot_info.memory_map_size = (UINT64)memory_map_size;
    g_boot_info.memory_map_descriptor_size = (UINT64)descriptor_size;
    return EFI_SUCCESS;
}

static EFI_STATUS exit_boot_services(EFI_HANDLE image_handle) {
    EFI_STATUS status = EFI_LOAD_ERROR;
    UINTN map_key = 0u;
    UINTN attempt;

    for (attempt = 0u; attempt < 2u; ++attempt) {
        status = capture_memory_map(&map_key);
        if (EFI_ERROR(status)) {
            return status;
        }

        status = uefi_call_wrapper(BS->ExitBootServices, 2, image_handle, map_key);
        if (!EFI_ERROR(status)) {
            return EFI_SUCCESS;
        }

        if (status != EFI_INVALID_PARAMETER) {
            return EFI_ABORTED;
        }
    }

    return status;
}

typedef void (*tinyos_kernel_entry_t)(const struct tinyos_boot_info *);

static void handoff_to_kernel(const struct tinyos_boot_info *boot_info, tinyos_kernel_entry_t kernel_entry) {
    __asm__ volatile (
        "mov %0, %%rdi\n\t"
        "jmp *%1\n\t"
        :
        : "r"(boot_info), "r"(kernel_entry)
        : "memory", "rdi"
    );
    __builtin_unreachable();
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    EFI_STATUS status;
    EFI_PHYSICAL_ADDRESS kernel_address = 0u;
    UINT64 kernel_size = 0u;
    tinyos_kernel_entry_t kernel_entry;

    if ((system_table == NULL) || (system_table->BootServices == NULL)) {
        return EFI_INVALID_PARAMETER;
    }

    InitializeLib(image_handle, system_table);
    debugcon_write('L');

    memory_zero(&g_boot_info, sizeof(g_boot_info));
    g_boot_info.revision = TINYOS_BOOT_INFO_REVISION;
    g_boot_info.boot_method = TINYOS_BOOT_METHOD_UEFI;
    copy_ascii(g_boot_info.boot_path, sizeof(g_boot_info.boot_path), "UEFI -> BOOTX64.EFI -> kernel_main");

    if (system_table->ConOut != NULL) {
        uefi_call_wrapper(system_table->ConOut->Reset, 2, system_table->ConOut, TRUE);
    }
    uefi_call_wrapper(BS->SetWatchdogTimer, 4, 0u, 0u, 0u, (CHAR16 *)0);

    write_ascii(system_table, "tinyOS UEFI loader starting...\n");

    status = load_kernel_image(image_handle, &kernel_address, &kernel_size);
    if (EFI_ERROR(status)) {
        write_ascii(system_table, "Failed to load KERNEL.BIN.\n");
        return status;
    }
    debugcon_write('P');
    if (*(volatile UINT32 *)(UINTN)kernel_address == 0xfa1e0ff3u) {
        debugcon_write('V');
    } else {
        debugcon_write('X');
    }

    capture_framebuffer(system_table);
    capture_acpi_rsdp(system_table);
    status = exit_boot_services(image_handle);
    if (EFI_ERROR(status)) {
        return status;
    }
    debugcon_write('E');

    kernel_entry = (tinyos_kernel_entry_t)(UINTN)kernel_address;
    handoff_to_kernel(&g_boot_info, kernel_entry);
    return EFI_ABORTED;
}
