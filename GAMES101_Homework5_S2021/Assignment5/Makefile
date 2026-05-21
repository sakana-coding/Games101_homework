BUILD_DIR := build

.PHONY: all run clean

all:
	cmake -B $(BUILD_DIR) -S . && cmake --build $(BUILD_DIR)

run: all
	./$(BUILD_DIR)/RayTracing

clean:
	rm -rf $(BUILD_DIR) *.ppm
