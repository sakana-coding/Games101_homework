.PHONY: all configure build run clean

BUILD_DIR := build
TARGET := RayTracing

all: build

configure:
	cmake -S . -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR) -j

run: build
	rm -f $(BUILD_DIR)/*.ppm
	cd $(BUILD_DIR) && ./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) *.ppm
