BUILD_DIR ?= build/x86_64
IMAGE := $(BUILD_DIR)/tinyos-x86_64.img
EFI_LOADER := $(BUILD_DIR)/esp/EFI/BOOT/BOOTX64.EFI
KERNEL_BIN := $(BUILD_DIR)/KERNEL.BIN

.PHONY: all build build-vbox run check-baseline check-ui check-image check-vbox clean

all: build

build:
	./scripts/build_x86_64.sh

build-vbox: build
	bash ./scripts/build_virtualbox_disk.sh

run: build
	./scripts/run_qemu_x86_64.sh

check-baseline: build
	./scripts/check_task8_baseline.sh

check-ui: build
	./scripts/check_lvgl_interaction.sh

check-image: build
	python3 ./scripts/check_uefi_image_layout.py "$(IMAGE)" "$(EFI_LOADER)" "$(KERNEL_BIN)"

check-vbox: build
	bash ./scripts/check_virtualbox_disk.sh

clean:
	rm -rf build
