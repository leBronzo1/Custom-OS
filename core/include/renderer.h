#ifndef RENDERER_H
#define RENDERER_H

#include <SDL.h>
#include <stdbool.h>

typedef struct {
    SDL_Window* win;
    SDL_Renderer* ren;
    SDL_Surface* winSurface;
} Renderer;

int init_sdl(Renderer* r);
int run_img(Renderer* r);
int run_loop(Renderer* r);
void shutdown_sdl(Renderer* r);


bool img_load();
bool img_init();
bool img_kill();

#endif