CC      = gcc
CFLAGS  = -Wall -O2 -std=c11 -Iinclude   # -Iinclude so #include "renderer.h" works
LIBS    = -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_mixer -lwiringPi

SRC_DIR   = src
BUILD_DIR = build

# Auto-find all .c files in src/
SRCS = $(wildcard $(SRC_DIR)/*.c)
# Turn src/main.c → build/main.o etc
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

OUT = myos

all: $(BUILD_DIR) $(OUT)

# Link all object files into final binary
$(OUT): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# Compile each .c to a .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Create build dir if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(OUT)

.PHONY: all clean