# ------------------------------------------------------------------
# myos Makefile
# Run from core/ with: make run
# ------------------------------------------------------------------

CC      ?= cc
OUT     ?= myos

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
PYTHON_CONFIG ?= python3-config
PY_CFLAGS     := $(shell $(PYTHON_CONFIG) --cflags 2>/dev/null)
PY_LIBS       := $(shell $(PYTHON_CONFIG) --embed --ldflags 2>/dev/null || $(PYTHON_CONFIG) --ldflags 2>/dev/null)

# ------------------------------------------------------------------
# Platform detection
# ------------------------------------------------------------------
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)

    BREW ?= brew
    SDL2_PREFIX       := $(shell $(BREW) --prefix sdl2 2>/dev/null)
    SDL2_TTF_PREFIX   := $(shell $(BREW) --prefix sdl2_ttf 2>/dev/null)
    SDL2_MIXER_PREFIX := $(shell $(BREW) --prefix sdl2_mixer 2>/dev/null)
    MGBA_PREFIX       := $(shell $(BREW) --prefix mgba 2>/dev/null)
    SDL2_CONFIG       ?= $(SDL2_PREFIX)/bin/sdl2-config

    SDL_CFLAGS := -I$(SDL2_PREFIX)/include/SDL2 \
                  $(shell $(SDL2_CONFIG) --cflags 2>/dev/null)
    SDL_LIBS   := -L$(SDL2_PREFIX)/lib \
                  $(shell $(SDL2_CONFIG) --libs 2>/dev/null) \
                  -L$(SDL2_TTF_PREFIX)/lib -lSDL2_ttf \
                  -L$(SDL2_MIXER_PREFIX)/lib -lSDL2_mixer

    MGBA_CFLAGS := -I$(MGBA_PREFIX)/include
    MGBA_LIBS   := -L$(MGBA_PREFIX)/lib -lmgba

else ifeq ($(UNAME_S),Linux)

    SDL2_CONFIG ?= sdl2-config

    SDL_CFLAGS := $(shell $(SDL2_CONFIG) --cflags 2>/dev/null)
    SDL_LIBS   := $(shell $(SDL2_CONFIG) --libs 2>/dev/null) \
                  -lSDL2_ttf \
                  -lSDL2_mixer

    MGBA_CFLAGS :=
    MGBA_LIBS   := -lmgba

else

    $(error Unsupported platform '$(UNAME_S)'; this Makefile supports macOS and Linux)

endif

# ------------------------------------------------------------------
# Compiler / linker flags
# ------------------------------------------------------------------
CPPFLAGS += -I$(INC_DIR) $(SDL_CFLAGS) $(MGBA_CFLAGS)
CFLAGS   += -Wall -Wextra -O2 -std=c11 -MMD -MP $(PY_CFLAGS)
LDLIBS   += $(SDL_LIBS) $(MGBA_LIBS) $(PY_LIBS) -lm

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
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)
	@echo ""
	@echo "  built: $(OUT)"
	@echo ""

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: all
	./$(OUT)

clean:
	rm -rf $(BUILD_DIR) $(OUT)

-include $(DEPS)

.PHONY: all run clean
