# ------------------------------------------------------------------
# myos Makefile
# Run from core/ with: make run
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
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

# ------------------------------------------------------------------
# Python
# ------------------------------------------------------------------
PY_CFLAGS = $(shell python3-config --cflags)
PY_LIBS   = $(shell python3-config --embed --ldflags 2>/dev/null || python3-config --ldflags)

# ------------------------------------------------------------------
# Platform detection
# ------------------------------------------------------------------
UNAME := $(shell uname)

ifeq ($(UNAME),Darwin)

    BREW_PREFIX := $(shell brew --prefix)

    SDL_CFLAGS = $(shell sdl2-config --cflags)
    SDL_LIBS   = $(shell sdl2-config --libs) \
                 -lSDL2_ttf \
                 -lSDL2_mixer

    MGBA_CFLAGS = -I$(BREW_PREFIX)/include
    MGBA_LIBS   = -L$(BREW_PREFIX)/lib -lmgba

    EXTRA_LIBS = -lm

else

    SDL_CFLAGS = $(shell sdl2-config --cflags)
    SDL_LIBS   = $(shell sdl2-config --libs) \
                 -lSDL2_ttf \
                 -lSDL2_mixer \
                 -lmgba

    EXTRA_LIBS = $(shell pkg-config --libs wiringPi 2>/dev/null) -lm

    MGBA_CFLAGS =
    MGBA_LIBS   =

endif

# ------------------------------------------------------------------
# Compiler / linker flags
# ------------------------------------------------------------------
CFLAGS = -Wall -Wextra -O2 -std=c11 \
          -MMD -MP \
          -I$(INC_DIR) \
          $(SDL_CFLAGS) \
          $(MGBA_CFLAGS) \
          $(PY_CFLAGS)

LIBS = $(SDL_LIBS) \
       $(MGBA_LIBS) \
       $(EXTRA_LIBS) \
       $(PY_LIBS)

# ------------------------------------------------------------------
# Targets
# ------------------------------------------------------------------
all: $(CJSON_H) $(CJSON_C) $(BUILD_DIR) $(OUT)

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

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: all
	./$(OUT)

clean:
	rm -rf $(BUILD_DIR) $(OUT)

-include $(DEPS)

.PHONY: all run clean