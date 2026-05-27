#include "renderer.h"

int init_sdl(SDL_Window** win, SDL_Renderer** ren) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 0;  // failure
    }

    *win = SDL_CreateWindow(
        "Hello SDL2",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_SHOWN
    );
    if (!*win) { SDL_Quit(); return 0; }

    *ren = SDL_CreateRenderer(*win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!*ren) { SDL_DestroyWindow(*win); SDL_Quit(); return 0; }

    return 1;  // success
}