BUILD_DIR ?= build/x86_64
IMAGE := $(BUILD_DIR)/tinyos-x86_64.img

.PHONY: all build run check-baseline clean

all: build

build: $(IMAGE)

$(IMAGE):
	./scripts/build_x86_64.sh

run: build
	./scripts/run_qemu_x86_64.sh

check-baseline: build
	./scripts/check_task8_baseline.sh

clean:
	rm -rf build
