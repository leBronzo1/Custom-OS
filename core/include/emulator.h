#ifndef EMULATOR_H
#define EMULATOR_H

#include <SDL.h>
#include <mgba/flags.h>
#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba-util/vfs.h>
#include <mgba/core/input.h>
#include <mgba/core/blip_buf.h>
#include <mgba/core/serialize.h>
#include <SDL_mixer.h>

/*
GBA Section
*/

// Runs a ROM — blocks until the user quits, then returns
int emulator_run(SDL_Renderer *ren, const char *rom_path);

// Save state to a file
void emulator_save_state(const char *path);

// Load state from a file
void emulator_load_state(const char *path);

#endif
