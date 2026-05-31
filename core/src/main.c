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
#include "../include/audio.h"
#include "../include/ipc.h"
#include "../include/state.h"
#include "../include/errors.h"

int main(int argc, char* argv[]) {
    Renderer renderer = {0};

    if (!init_sdl(&renderer)) {
        return 1;
    }

    int err = run_loop(&renderer);
    error_handling(err);
    shutdown_sdl(&renderer);

    return 0;
}