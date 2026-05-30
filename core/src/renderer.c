#include "renderer.h"
#include <stdio.h>

int init_sdl(Renderer* r) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 0;
    }

    r->win = SDL_CreateWindow(
        "Hello SDL2",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_SHOWN
    );

    if (!r->win) {
        SDL_Quit();
        return 0;
    }

    r->ren = SDL_CreateRenderer(
        r->win,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!r->ren) {
        SDL_DestroyWindow(r->win);
        SDL_Quit();
        return 0;
    }

    return 1;
}

void run_loop(Renderer* r) {
    SDL_Event e;
    int running = 1;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)   // inline for now
                running = 0;
        }

        SDL_SetRenderDrawColor(r->ren, 15, 20, 40, 255);
        SDL_RenderClear(r->ren);

        SDL_RenderPresent(r->ren);
    }
}

void shutdown_sdl(Renderer* r) {
    SDL_DestroyRenderer(r->ren);
    SDL_DestroyWindow(r->win);
    SDL_Quit();
}