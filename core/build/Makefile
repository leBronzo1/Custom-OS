# ------------------------------------------------------------------
# myos Makefile
# Run from core/ with: make -f build/Makefile run
# ------------------------------------------------------------------

CC      = gcc
OUT     = myos

SRC_DIR   = src
INC_DIR   = include
BUILD_DIR = build

CJSON_H   = $(INC_DIR)/cJSON.h
CJSON_C   = $(SRC_DIR)/cJSON.c
CJSON_URL = https://raw.githubusercontent.com/DaveGamble/cJSON/master

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# ------------------------------------------------------------------
# Platform detection
# ------------------------------------------------------------------
UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
    SDL_CFLAGS = $(shell sdl2-config --cflags)
    SDL_LIBS   = $(shell sdl2-config --libs) -lSDL2_ttf -lSDL2_mixer -lmgba
    EXTRA_LIBS =
else
    SDL_CFLAGS = $(shell sdl2-config --cflags)
    SDL_LIBS   = $(shell sdl2-config --libs) -lSDL2_ttf -lSDL2_mixer -lmgba
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
all: $(CJSON_H) $(CJSON_C) $(BUILD_DIR) $(OUT)

# Fetch cJSON if missing
$(CJSON_H):
	@echo "  fetching cJSON.h..."
	@curl -fsSL $(CJSON_URL)/cJSON.h -o $@

$(CJSON_C):
	@echo "  fetching cJSON.c..."
	@curl -fsSL $(CJSON_URL)/cJSON.c -o $@

$(OUT): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)
	@echo ""
	@echo "  built: $(OUT)"
	@echo ""

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: all
	./$(OUT)

clean:
	rm -rf $(BUILD_DIR) $(OUT)

.PHONY: all run clean