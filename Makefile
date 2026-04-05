# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
CFLAGS += -Wno-unused-function

# Raylib paths for macOS
RAYLIB_INCLUDE = -I/opt/homebrew/include -I/usr/local/include
RAYLIB_LIB = -L/opt/homebrew/lib -L/usr/local/lib

# Link flags
LDFLAGS = $(RAYLIB_LIB) -lraylib -lm \
          -framework CoreVideo \
          -framework IOKit \
          -framework Cocoa \
          -framework GLUT \
          -framework OpenGL

# Directories (relative paths)
SRC_DIR   = src/2d_game
BUILD_DIR = build/2d_game

# Target executable
TARGET = $(BUILD_DIR)/duck_quest

# Object files
OBJS = \
	$(BUILD_DIR)/main.o \
	$(BUILD_DIR)/entities.o \
	$(BUILD_DIR)/systems.o \
	$(BUILD_DIR)/dungeon.o \
	$(BUILD_DIR)/renderer.o \
	$(BUILD_DIR)/combat.o \
	$(BUILD_DIR)/items.o \
	$(BUILD_DIR)/boss.o \
	$(BUILD_DIR)/minimap.o

# Default target
all: $(TARGET)

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link
$(TARGET): $(BUILD_DIR) $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete! Run with: $(TARGET)"

# Compile rules
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(SRC_DIR)/types.h $(SRC_DIR)/utils.h $(SRC_DIR)/entities.h $(SRC_DIR)/systems.h $(SRC_DIR)/dungeon.h $(SRC_DIR)/renderer.h $(SRC_DIR)/combat.h $(SRC_DIR)/items.h $(SRC_DIR)/boss.h $(SRC_DIR)/minimap.h
	$(CC) $(CFLAGS) $(RAYLIB_INCLUDE) -c $< -o $@

$(BUILD_DIR)/entities.o: $(SRC_DIR)/entities.c $(SRC_DIR)/entities.h $(SRC_DIR)/types.h $(SRC_DIR)/utils.h $(SRC_DIR)/combat.h $(SRC_DIR)/dungeon.h $(SRC_DIR)/items.h
	$(CC) $(CFLAGS) $(RAYLIB_INCLUDE) -c $< -o $@

$(BUILD_DIR)/systems.o: $(SRC_DIR)/systems.c $(SRC_DIR)/systems.h $(SRC_DIR)/types.h $(SRC_DIR)/utils.h $(SRC_DIR)/renderer.h $(SRC_DIR)/dungeon.h $(SRC_DIR)/minimap.h
	$(CC) $(CFLAGS) $(RAYLIB_INCLUDE) -c $< -o $@

$(BUILD_DIR)/dungeon.o: $(SRC_DIR)/dungeon.c $(SRC_DIR)/dungeon.h $(SRC_DIR)/types.h $(SRC_DIR)/utils.h $(SRC_DIR)/combat.h $(SRC_DIR)/items.h $(SRC_DIR)/boss.h
	$(CC) $(CFLAGS) $(RAYLIB_INCLUDE) -c $< -o $@

$(BUILD_DIR)/renderer.o: $(SRC_DIR)/renderer.c $(SRC_DIR)/renderer.h $(SRC_DIR)/types.h $(SRC_DIR)/utils.h
	$(CC) $(CFLAGS) $(RAYLIB_INCLUDE) -c $< -o $@

$(BUILD_DIR)/combat.o: $(SRC_DIR)/combat.c $(SRC_DIR)/combat.h $(SRC_DIR)/types.h $(SRC_DIR)/utils.h $(SRC_DIR)/items.h
	$(CC) $(CFLAGS) $(RAYLIB_INCLUDE) -c $< -o $@

$(BUILD_DIR)/items.o: $(SRC_DIR)/items.c $(SRC_DIR)/items.h $(SRC_DIR)/types.h $(SRC_DIR)/utils.h $(SRC_DIR)/renderer.h
	$(CC) $(CFLAGS) $(RAYLIB_INCLUDE) -c $< -o $@

$(BUILD_DIR)/boss.o: $(SRC_DIR)/boss.c $(SRC_DIR)/boss.h $(SRC_DIR)/types.h $(SRC_DIR)/utils.h $(SRC_DIR)/combat.h
	$(CC) $(CFLAGS) $(RAYLIB_INCLUDE) -c $< -o $@

$(BUILD_DIR)/minimap.o: $(SRC_DIR)/minimap.c $(SRC_DIR)/minimap.h $(SRC_DIR)/types.h $(SRC_DIR)/utils.h
	$(CC) $(CFLAGS) $(RAYLIB_INCLUDE) -c $< -o $@

# Clean
clean:
	rm -rf $(BUILD_DIR)
	@echo "Cleaned build artifacts"

# Rebuild
rebuild: clean all

# Run
run: all
	$(TARGET)

.PHONY: all clean rebuild run
