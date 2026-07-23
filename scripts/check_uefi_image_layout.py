#!/usr/bin/env python3
import math
import struct
import sys
from pathlib import Path


MBR_SIGNATURE_OFFSET = 510
PARTITION_ENTRY_OFFSET = 446
BYTES_PER_DIR_ENTRY = 32
FAT16_END_OF_CHAIN = 0xFFF8


def read_u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def read_u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def fail(message: str) -> int:
    print(f"[FAIL] {message}", file=sys.stderr)
    return 1


def pass_line(message: str) -> None:
    print(f"[PASS] {message}")


def parse_name(name_83: bytes) -> str:
    stem = name_83[:8].decode("ascii", errors="replace").rstrip()
    ext = name_83[8:11].decode("ascii", errors="replace").rstrip()
    return f"{stem}.{ext}" if ext else stem


def parse_directory_entries(directory_data: bytes) -> list[dict[str, int | str | bytes]]:
    entries: list[dict[str, int | str | bytes]] = []
    for offset in range(0, len(directory_data), BYTES_PER_DIR_ENTRY):
        entry = directory_data[offset:offset + BYTES_PER_DIR_ENTRY]
        if len(entry) < BYTES_PER_DIR_ENTRY:
            break
        first_byte = entry[0]
        if first_byte == 0x00:
            break
        if first_byte == 0xE5:
            continue
        attributes = entry[11]
        if attributes == 0x0F:
            continue
        name_83 = entry[0:11]
        entries.append(
            {
                "name_83": name_83,
                "name": parse_name(name_83),
                "attributes": attributes,
                "cluster": read_u16(entry, 26),
                "size": read_u32(entry, 28),
            }
        )
    return entries


def find_entry(entries: list[dict[str, int | str | bytes]], expected_name: bytes) -> dict[str, int | str | bytes] | None:
    for entry in entries:
        if entry["name_83"] == expected_name:
            return entry
    return None


def read_cluster(partition: bytes, cluster_number: int, bytes_per_sector: int, sectors_per_cluster: int, first_data_sector: int) -> bytes:
    if cluster_number < 2:
        raise ValueError(f"invalid cluster number: {cluster_number}")
    cluster_size = bytes_per_sector * sectors_per_cluster
    sector_index = first_data_sector + (cluster_number - 2) * sectors_per_cluster
    start = sector_index * bytes_per_sector
    end = start + cluster_size
    return partition[start:end]


def read_file_by_chain(
    partition: bytes,
    fat: bytes,
    start_cluster: int,
    file_size: int,
    bytes_per_sector: int,
    sectors_per_cluster: int,
    first_data_sector: int,
) -> bytes:
    if start_cluster < 2:
        raise ValueError(f"invalid start cluster: {start_cluster}")

    cluster_size = bytes_per_sector * sectors_per_cluster
    remaining = file_size
    current_cluster = start_cluster
    visited: set[int] = set()
    chunks: list[bytes] = []

    while remaining > 0:
        if current_cluster in visited:
            raise ValueError(f"detected FAT cycle at cluster {current_cluster}")
        visited.add(current_cluster)
        cluster_data = read_cluster(partition, current_cluster, bytes_per_sector, sectors_per_cluster, first_data_sector)
        chunks.append(cluster_data[: min(len(cluster_data), remaining)])
        remaining -= min(cluster_size, remaining)
        next_cluster = read_u16(fat, current_cluster * 2)
        if remaining <= 0:
            break
        if next_cluster < 2 or (next_cluster >= FAT16_END_OF_CHAIN and remaining > 0):
            raise ValueError(f"unexpected FAT chain termination after cluster {current_cluster}")
        current_cluster = next_cluster

    return b"".join(chunks)


def main(argv: list[str]) -> int:
    if len(argv) not in (2, 4):
        print("usage: check_uefi_image_layout.py <disk.img> [<bootx64.efi> <kernel.bin>]", file=sys.stderr)
        return 1

    image_path = Path(argv[1])
    if not image_path.is_file():
        return fail(f"disk image not found: {image_path}")

    bootx64_path = Path(argv[2]) if len(argv) == 4 else None
    kernel_path = Path(argv[3]) if len(argv) == 4 else None
    if bootx64_path is not None and not bootx64_path.is_file():
        return fail(f"BOOTX64.EFI artifact not found: {bootx64_path}")
    if kernel_path is not None and not kernel_path.is_file():
        return fail(f"KERNEL.BIN artifact not found: {kernel_path}")

    image = image_path.read_bytes()
    if len(image) < 512:
        return fail("disk image is too small to contain an MBR")

    if image[MBR_SIGNATURE_OFFSET:MBR_SIGNATURE_OFFSET + 2] != b"\x55\xAA":
        return fail("missing MBR signature")
    pass_line("MBR signature present")

    partition_entry = image[PARTITION_ENTRY_OFFSET:PARTITION_ENTRY_OFFSET + 16]
    partition_type = partition_entry[4]
    start_lba = read_u32(partition_entry, 8)
    total_sectors = read_u32(partition_entry, 12)
    if partition_type != 0xEF:
        return fail(f"unexpected partition type: 0x{partition_type:02x}")
    if start_lba == 0 or total_sectors == 0:
        return fail("partition entry is missing start LBA or size")
    pass_line("EFI System Partition entry present in MBR")

    partition_start = start_lba * 512
    partition_end = partition_start + total_sectors * 512
    if partition_end > len(image):
        return fail("partition extends past end of disk image")
    partition = image[partition_start:partition_end]

    if partition[MBR_SIGNATURE_OFFSET:MBR_SIGNATURE_OFFSET + 2] != b"\x55\xAA":
        return fail("missing FAT boot-sector signature in ESP")
    pass_line("ESP boot-sector signature present")

    bytes_per_sector = read_u16(partition, 11)
    sectors_per_cluster = partition[13]
    reserved_sectors = read_u16(partition, 14)
    fat_count = partition[16]
    root_entry_count = read_u16(partition, 17)
    sectors_per_fat = read_u16(partition, 22)
    filesystem_type = partition[54:62].decode("ascii", errors="replace").rstrip()
    volume_label = partition[43:54].decode("ascii", errors="replace").rstrip()

    if bytes_per_sector == 0 or sectors_per_cluster == 0 or fat_count == 0 or sectors_per_fat == 0:
        return fail("ESP boot sector has invalid FAT geometry")
    if filesystem_type != "FAT16":
        return fail(f"unexpected filesystem type: {filesystem_type!r}")
    pass_line(f"ESP FAT geometry parsed: label={volume_label}, fs={filesystem_type}")

    fat_size = sectors_per_fat * bytes_per_sector
    fat_offset = reserved_sectors * bytes_per_sector
    fat_primary = partition[fat_offset:fat_offset + fat_size]
    fat_secondary = partition[fat_offset + fat_size:fat_offset + fat_size * 2]
    if len(fat_primary) != fat_size or len(fat_secondary) != fat_size:
        return fail("ESP FAT copies are truncated")
    if fat_primary != fat_secondary:
        return fail("primary and secondary FAT copies differ")
    pass_line("Primary and secondary FAT copies match")

    root_dir_sectors = math.ceil(root_entry_count * BYTES_PER_DIR_ENTRY / bytes_per_sector)
    first_root_sector = reserved_sectors + fat_count * sectors_per_fat
    first_data_sector = first_root_sector + root_dir_sectors
    root_offset = first_root_sector * bytes_per_sector
    root_size = root_dir_sectors * bytes_per_sector
    root_entries = parse_directory_entries(partition[root_offset:root_offset + root_size])

    efi_entry = find_entry(root_entries, b"EFI        ")
    kernel_root_entry = find_entry(root_entries, b"KERNEL  BIN")
    if efi_entry is None or kernel_root_entry is None:
        return fail("ESP root directory is missing EFI or KERNEL.BIN")
    pass_line("ESP root directory contains EFI and KERNEL.BIN")

    if (int(efi_entry["attributes"]) & 0x10) == 0:
        return fail("EFI entry is not a directory")
    if int(kernel_root_entry["size"]) == 0:
        return fail("root KERNEL.BIN has zero size")

    efi_dir = read_cluster(partition, int(efi_entry["cluster"]), bytes_per_sector, sectors_per_cluster, first_data_sector)
    efi_entries = parse_directory_entries(efi_dir)
    boot_entry = find_entry(efi_entries, b"BOOT       ")
    if boot_entry is None or (int(boot_entry["attributes"]) & 0x10) == 0:
        return fail("EFI directory is missing BOOT subdirectory")
    pass_line("EFI directory contains BOOT subdirectory")

    boot_dir = read_cluster(partition, int(boot_entry["cluster"]), bytes_per_sector, sectors_per_cluster, first_data_sector)
    boot_entries = parse_directory_entries(boot_dir)
    bootx64_entry = find_entry(boot_entries, b"BOOTX64 EFI")
    kernel_boot_entry = find_entry(boot_entries, b"KERNEL  BIN")
    if bootx64_entry is None or kernel_boot_entry is None:
        return fail("EFI/BOOT directory is missing BOOTX64.EFI or KERNEL.BIN")
    if int(bootx64_entry["size"]) == 0:
        return fail("BOOTX64.EFI has zero size")
    if int(kernel_boot_entry["size"]) == 0:
        return fail("EFI/BOOT/KERNEL.BIN has zero size")
    pass_line("EFI/BOOT contains BOOTX64.EFI and KERNEL.BIN")

    try:
        image_bootx64 = read_file_by_chain(
            partition,
            fat_primary,
            int(bootx64_entry["cluster"]),
            int(bootx64_entry["size"]),
            bytes_per_sector,
            sectors_per_cluster,
            first_data_sector,
        )
        image_kernel = read_file_by_chain(
            partition,
            fat_primary,
            int(kernel_boot_entry["cluster"]),
            int(kernel_boot_entry["size"]),
            bytes_per_sector,
            sectors_per_cluster,
            first_data_sector,
        )
    except ValueError as exc:
        return fail(str(exc))

    if len(image_bootx64) != int(bootx64_entry["size"]):
        return fail("BOOTX64.EFI extracted size does not match directory entry")
    if len(image_kernel) != int(kernel_boot_entry["size"]):
        return fail("EFI/BOOT/KERNEL.BIN extracted size does not match directory entry")
    pass_line("FAT chains for BOOTX64.EFI and KERNEL.BIN are readable")

    if int(kernel_root_entry["size"]) != int(kernel_boot_entry["size"]):
        return fail("root KERNEL.BIN and EFI/BOOT/KERNEL.BIN sizes differ")
    pass_line("Root KERNEL.BIN and EFI/BOOT/KERNEL.BIN sizes match")

    if bootx64_path is not None and kernel_path is not None:
        expected_bootx64 = bootx64_path.read_bytes()
        expected_kernel = kernel_path.read_bytes()
        if image_bootx64 != expected_bootx64:
            return fail("BOOTX64.EFI content in disk image differs from build artifact")
        if image_kernel != expected_kernel:
            return fail("KERNEL.BIN content in disk image differs from build artifact")
        pass_line("Disk image BOOTX64.EFI matches build artifact")
        pass_line("Disk image KERNEL.BIN matches build artifact")

    print("== UEFI image layout check passed ==")
    print(f"image: {image_path}")
    print(f"partition start LBA: {start_lba}")
    print(f"partition sectors: {total_sectors}")
    print(f"volume label: {volume_label}")
    print(f"root kernel size: {kernel_root_entry['size']}")
    print(f"BOOTX64.EFI size: {bootx64_entry['size']}")
    if bootx64_path is not None and kernel_path is not None:
        print(f"artifact BOOTX64.EFI: {bootx64_path}")
        print(f"artifact KERNEL.BIN: {kernel_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
