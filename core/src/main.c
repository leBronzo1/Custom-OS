#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
    #include <SDL.h>
    #include <SDL_ttf.h>
    #include <SDL_image.h>
    #include <SDL_mixer.h>
#else
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_ttf.h>
    #include <SDL2/SDL_image.h>
    #include <SDL2/SDL_mixer.h>
#endif

#include "../include/types.h"
#include "../include/renderer.h"
#include "../include/input.h"
#include "../include/audio.h"
#include "../include/ipc.h"
#include "../include/state.h"

int main() {
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) != 0) {
        fprintf(stderr, "[myos] SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    return 0;
}