#ifndef EMULATOR_H
#define EMULATOR_H

#include <SDL.h>
#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba-util/vfs.h>
#include <mgba/core/input.h>
#include <mgba/core/blip_buf.h>
#include <mgba/core/serialize.h>

/*
GBA Section
*/

// Runs a ROM — blocks until the user quits, then returns
int emulator_run(SDL_Renderer *ren, const char *rom_path);

// Save state to a file
void emulator_save_state(const char *path);

// Load state from a file
void emulator_load_state(const char *path);

// Polls keyboard and returns GBA key bitmask
static uint32_t emulator_poll_input(void);

// Opens SDL audio device and sets up blip_buf rates
static void emulator_audio_init(struct mCore *core);

// Drains blip_buf samples into SDL audio queue each frame
static void emulator_audio_pump(struct mCore *core);

#endif