BUILD_DIR ?= build/x86_64
IMAGE := $(BUILD_DIR)/tinyos-x86_64.img
EFI_LOADER := $(BUILD_DIR)/esp/EFI/BOOT/BOOTX64.EFI
KERNEL_BIN := $(BUILD_DIR)/KERNEL.BIN

.PHONY: all build build-vbox build-aarch64 build-riscv64 run run-aarch64 run-riscv64 check-baseline check-ui check-image check-vbox check-aarch64 check-riscv64 check-multiarch-preflight clean

all: build

build:
	./scripts/build_x86_64.sh

build-vbox: build
	bash ./scripts/build_virtualbox_disk.sh

build-aarch64:
	./scripts/build_aarch64_virt.sh

build-riscv64:
	./scripts/build_riscv64_virt.sh

run: build
	./scripts/run_qemu_x86_64.sh

run-aarch64: build-aarch64
	./scripts/run_qemu_aarch64_virt.sh

run-riscv64: build-riscv64
	./scripts/run_qemu_riscv64_virt.sh

check-baseline: build
	./scripts/check_task8_baseline.sh

check-ui: build
	./scripts/check_lvgl_interaction.sh

check-aarch64:
	./scripts/check_aarch64_virt.sh

check-riscv64:
	./scripts/check_riscv64_virt.sh

check-multiarch-preflight:
	./scripts/check_multiarch_preflight.sh

check-image: build
	python3 ./scripts/check_uefi_image_layout.py "$(IMAGE)" "$(EFI_LOADER)" "$(KERNEL_BIN)"

check-vbox: build
	bash ./scripts/check_virtualbox_disk.sh

clean:
	rm -rf build
