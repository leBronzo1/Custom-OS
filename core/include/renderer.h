#ifndef RENDERER_H
#define RENDERER_H

#include <SDL.h>

typedef struct {
    SDL_Window* win;
    SDL_Renderer* ren;
} Renderer;

int  init_sdl(Renderer* r);
void run_loop(Renderer* r);
void shutdown_sdl(Renderer* r);

#endif