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

int main(int argc, char* argv[]) {
    SDL_Window*   win = NULL;
    SDL_Renderer* ren = NULL;

    if (!init_sdl(&win, &ren)) return 1;

    // event loop here, win and ren are usable
    // ...

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}