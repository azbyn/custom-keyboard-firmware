ROOT_DIR := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR := ${ROOT_DIR}/build

.PHONY: default
default: build

.PHONY: gen_cmake
gen_cmake:
	rm -rf ${BUILD_DIR}
	cmake -B ${BUILD_DIR} -S ${ROOT_DIR}/src -G Ninja

.PHONY: serial
serial:
	minicom -D /dev/ttyACM0


.PHONY: build
build:
	ninja  -C ${BUILD_DIR}

.PHONY: clean
clean:
	ninja  -C ${BUILD_DIR} clean

.PHONY: upload
upload:
	picotool load ${BUILD_DIR}/azbyn_keyboard.elf
	picotool reboot


#${MAKE} -C ${BUILD_DIR}
