#ifndef RENDERER_H
#define RENDERER_H

#include <SDL.h>
#include <stdbool.h>

typedef struct {
    SDL_Window* win;
    SDL_Renderer* ren;
    SDL_Texture* image1;
    char current_cover[256];
} Renderer;

int init_sdl(Renderer* r);
int run_loop(Renderer* r);
void shutdown_sdl(Renderer* r);

bool img_load(Renderer* r, const char* path);
bool img_init();
bool img_kill(Renderer* r);

#endif