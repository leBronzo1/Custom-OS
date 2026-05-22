# ------------------------------------------------------------------
# myos Makefile
# Works on macOS (dev) and Raspberry Pi (target)
# Usage:
#   make          — build the binary
#   make clean    — remove build artifacts
#   make run      — build and run
# ------------------------------------------------------------------

CC      = gcc
OUT     = myos

SRC_DIR   = src
INC_DIR   = include
BUILD_DIR = build

# Auto-find all .c files in src/
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# ------------------------------------------------------------------
# Platform detection
# ------------------------------------------------------------------
UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
    # macOS — sdl2-config handles include + lib paths from Homebrew
    SDL_CFLAGS = $(shell sdl2-config --cflags)
    SDL_LIBS   = $(shell sdl2-config --libs) -lSDL2_ttf -lSDL2_image -lSDL2_mixer
    EXTRA_LIBS =
else
    # Raspberry Pi / Linux
    SDL_CFLAGS = $(shell sdl2-config --cflags)
    SDL_LIBS   = $(shell sdl2-config --libs) -lSDL2_ttf -lSDL2_image -lSDL2_mixer
    EXTRA_LIBS = -lwiringPi -lm
endif

# ------------------------------------------------------------------
# Flags
# ------------------------------------------------------------------
CFLAGS  = -Wall -Wextra -O2 -std=c11 -I$(INC_DIR) $(SDL_CFLAGS)
LIBS    = $(SDL_LIBS) $(EXTRA_LIBS)

# ------------------------------------------------------------------
# Targets
# ------------------------------------------------------------------
all: $(BUILD_DIR) $(OUT)

# Link all .o files into the final binary
$(OUT): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)
	@echo ""
	@echo "  built: $(OUT)"
	@echo ""

# Compile each .c -> .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Create build dir if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build and run
run: all
	./$(OUT)

# Remove all build artifacts
clean:
	rm -rf $(BUILD_DIR) $(OUT)

.PHONY: all run clean