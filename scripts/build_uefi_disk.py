#!/usr/bin/env python3
import math
import struct
import sys
from pathlib import Path


BYTES_PER_SECTOR = 512
SECTORS_PER_CLUSTER = 1
RESERVED_SECTORS = 1
FAT_COUNT = 2
ROOT_ENTRY_COUNT = 512
PARTITION_START_LBA = 2048
PARTITION_SECTORS = 32768
DISK_SECTORS = PARTITION_START_LBA + PARTITION_SECTORS
MEDIA_DESCRIPTOR = 0xF8
FAT16_EOC = 0xFFFF


def dos_name(name: str) -> bytes:
    stem, ext = name.split(".", 1) if "." in name else (name, "")
    stem = stem.upper()[:8].ljust(8)
    ext = ext.upper()[:3].ljust(3)
    return (stem + ext).encode("ascii")


def root_dir_sectors() -> int:
    return (ROOT_ENTRY_COUNT * 32 + BYTES_PER_SECTOR - 1) // BYTES_PER_SECTOR


def compute_fat_sectors() -> tuple[int, int]:
    sectors_per_fat = 1
    while True:
        data_sectors = PARTITION_SECTORS - RESERVED_SECTORS - FAT_COUNT * sectors_per_fat - root_dir_sectors()
        total_clusters = data_sectors // SECTORS_PER_CLUSTER
        required = math.ceil(((total_clusters + 2) * 2) / BYTES_PER_SECTOR)
        if required == sectors_per_fat:
            return sectors_per_fat, total_clusters
        sectors_per_fat = required


def directory_entry(name_83: bytes, attributes: int, start_cluster: int, file_size: int) -> bytes:
    return struct.pack(
        "<11sBBBHHHHHHHI",
        name_83,
        attributes,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        start_cluster & 0xFFFF,
        file_size,
    )


def build_directory_cluster(entries: list[bytes], cluster_size: int) -> bytes:
    data = bytearray(cluster_size)
    offset = 0
    for entry in entries:
        data[offset:offset + 32] = entry
        offset += 32
    return bytes(data)


def write_chain(volume: bytearray, first_data_offset: int, cluster_size: int, start_cluster: int, data: bytes) -> None:
    if not data:
        return

    total_clusters = math.ceil(len(data) / cluster_size)
    for index in range(total_clusters):
        cluster_number = start_cluster + index
        cluster_offset = first_data_offset + (cluster_number - 2) * cluster_size
        chunk = data[index * cluster_size:(index + 1) * cluster_size]
        volume[cluster_offset:cluster_offset + len(chunk)] = chunk


def build_fat16_volume(efi_binary: bytes, kernel_binary: bytes) -> bytes:
    sectors_per_fat, total_clusters = compute_fat_sectors()
    if total_clusters < 4085 or total_clusters >= 65525:
        raise RuntimeError(f"invalid FAT16 cluster count: {total_clusters}")

    cluster_size = BYTES_PER_SECTOR * SECTORS_PER_CLUSTER
    first_root_sector = RESERVED_SECTORS + FAT_COUNT * sectors_per_fat
    first_data_sector = first_root_sector + root_dir_sectors()
    volume_size = PARTITION_SECTORS * BYTES_PER_SECTOR
    volume = bytearray(volume_size)

    efi_file_clusters = max(1, math.ceil(len(efi_binary) / cluster_size))
    kernel_file_clusters = max(1, math.ceil(len(kernel_binary) / cluster_size))

    efi_dir_cluster = 2
    boot_dir_cluster = 3
    bootx64_start_cluster = 4
    kernel_start_cluster = bootx64_start_cluster + efi_file_clusters

    fat_entries = [0] * (total_clusters + 2)
    fat_entries[0] = 0xFFF8
    fat_entries[1] = FAT16_EOC
    fat_entries[efi_dir_cluster] = FAT16_EOC
    fat_entries[boot_dir_cluster] = FAT16_EOC

    for index in range(efi_file_clusters):
        cluster = bootx64_start_cluster + index
        fat_entries[cluster] = FAT16_EOC if index == efi_file_clusters - 1 else cluster + 1

    for index in range(kernel_file_clusters):
        cluster = kernel_start_cluster + index
        fat_entries[cluster] = FAT16_EOC if index == kernel_file_clusters - 1 else cluster + 1

    fat_size_bytes = sectors_per_fat * BYTES_PER_SECTOR
    fat_bytes = bytearray(fat_size_bytes)
    for index, value in enumerate(fat_entries):
        struct.pack_into("<H", fat_bytes, index * 2, value)

    fat1_offset = RESERVED_SECTORS * BYTES_PER_SECTOR
    fat2_offset = fat1_offset + fat_size_bytes
    volume[fat1_offset:fat1_offset + fat_size_bytes] = fat_bytes
    volume[fat2_offset:fat2_offset + fat_size_bytes] = fat_bytes

    root_offset = first_root_sector * BYTES_PER_SECTOR
    root_entries = [
        directory_entry(b"EFI        ", 0x10, efi_dir_cluster, 0),
        directory_entry(dos_name("KERNEL.BIN"), 0x20, kernel_start_cluster, len(kernel_binary)),
    ]
    volume[root_offset:root_offset + len(root_entries) * 32] = b"".join(root_entries)

    efi_dir_entries = [
        directory_entry(b".          ", 0x10, efi_dir_cluster, 0),
        directory_entry(b"..         ", 0x10, 0, 0),
        directory_entry(b"BOOT       ", 0x10, boot_dir_cluster, 0),
    ]
    boot_dir_entries = [
        directory_entry(b".          ", 0x10, boot_dir_cluster, 0),
        directory_entry(b"..         ", 0x10, efi_dir_cluster, 0),
        directory_entry(dos_name("BOOTX64.EFI"), 0x20, bootx64_start_cluster, len(efi_binary)),
        directory_entry(dos_name("KERNEL.BIN"), 0x20, kernel_start_cluster, len(kernel_binary)),
    ]

    first_data_offset = first_data_sector * BYTES_PER_SECTOR
    write_chain(volume, first_data_offset, cluster_size, efi_dir_cluster, build_directory_cluster(efi_dir_entries, cluster_size))
    write_chain(volume, first_data_offset, cluster_size, boot_dir_cluster, build_directory_cluster(boot_dir_entries, cluster_size))
    write_chain(volume, first_data_offset, cluster_size, bootx64_start_cluster, efi_binary)
    write_chain(volume, first_data_offset, cluster_size, kernel_start_cluster, kernel_binary)

    boot_sector = bytearray(BYTES_PER_SECTOR)
    boot_sector[0:3] = b"\xEB\x3C\x90"
    boot_sector[3:11] = b"MSWIN4.1"
    struct.pack_into("<H", boot_sector, 11, BYTES_PER_SECTOR)
    boot_sector[13] = SECTORS_PER_CLUSTER
    struct.pack_into("<H", boot_sector, 14, RESERVED_SECTORS)
    boot_sector[16] = FAT_COUNT
    struct.pack_into("<H", boot_sector, 17, ROOT_ENTRY_COUNT)
    struct.pack_into("<H", boot_sector, 19, PARTITION_SECTORS)
    boot_sector[21] = MEDIA_DESCRIPTOR
    struct.pack_into("<H", boot_sector, 22, sectors_per_fat)
    struct.pack_into("<H", boot_sector, 24, 63)
    struct.pack_into("<H", boot_sector, 26, 255)
    struct.pack_into("<I", boot_sector, 28, PARTITION_START_LBA)
    struct.pack_into("<I", boot_sector, 32, 0)
    boot_sector[36] = 0x80
    boot_sector[38] = 0x29
    struct.pack_into("<I", boot_sector, 39, 0x54494E59)
    boot_sector[43:54] = b"TINYOSUEFI "
    boot_sector[54:62] = b"FAT16   "
    boot_sector[510:512] = b"\x55\xAA"
    volume[0:BYTES_PER_SECTOR] = boot_sector

    return bytes(volume)


def build_mbr() -> bytes:
    mbr = bytearray(BYTES_PER_SECTOR)
    entry_offset = 446
    mbr[entry_offset + 0] = 0x80
    mbr[entry_offset + 1:entry_offset + 4] = b"\xFE\xFF\xFF"
    mbr[entry_offset + 4] = 0xEF
    mbr[entry_offset + 5:entry_offset + 8] = b"\xFE\xFF\xFF"
    struct.pack_into("<I", mbr, entry_offset + 8, PARTITION_START_LBA)
    struct.pack_into("<I", mbr, entry_offset + 12, PARTITION_SECTORS)
    mbr[510:512] = b"\x55\xAA"
    return bytes(mbr)


def main(argv: list[str]) -> int:
    if len(argv) != 5:
        print(
            "usage: build_uefi_disk.py <bootx64.efi> <kernel.bin> <esp.img> <disk.img>",
            file=sys.stderr,
        )
        return 1

    efi_path = Path(argv[1])
    kernel_path = Path(argv[2])
    esp_path = Path(argv[3])
    disk_path = Path(argv[4])

    efi_binary = efi_path.read_bytes()
    kernel_binary = kernel_path.read_bytes()
    esp_image = build_fat16_volume(efi_binary, kernel_binary)

    disk_image = bytearray(DISK_SECTORS * BYTES_PER_SECTOR)
    disk_image[0:BYTES_PER_SECTOR] = build_mbr()
    partition_offset = PARTITION_START_LBA * BYTES_PER_SECTOR
    disk_image[partition_offset:partition_offset + len(esp_image)] = esp_image

    esp_path.write_bytes(esp_image)
    disk_path.write_bytes(disk_image)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
