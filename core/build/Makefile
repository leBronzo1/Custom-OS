CC     = gcc
CFLAGS = -Wall -O2 -std=c11 -Iinclude
OUT    = myos

SRC_DIR   = src
BUILD_DIR = build
SRCS      = $(wildcard $(SRC_DIR)/*.c)
OBJS      = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Detect OS
UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
    # macOS — homebrew SDL2 paths
    CFLAGS += $(shell sdl2-config --cflags)
    LIBS    = $(shell sdl2-config --libs) -lSDL2_ttf -lSDL2_image -lSDL2_mixer
else
    # Linux / Raspberry Pi
    CFLAGS += $(shell sdl2-config --cflags)
    LIBS    = $(shell sdl2-config --libs) -lSDL2_ttf -lSDL2_image -lSDL2_mixer -lwiringPi
endif

all: $(BUILD_DIR) $(OUT)

$(OUT): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(OUT)