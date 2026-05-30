#include "renderer.h"
#include "input.h"
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
        SDL_DestroyWindow(r->win);
        SDL_Quit();
        return 1;
    }

    r->ren = SDL_CreateRenderer(
        r->win,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!r->ren) {
        SDL_DestroyWindow(r->win);
        SDL_Quit();
        return 2;
    }

    return 0;
}

int run_img(Renderer* r) {
    if (!img_init()) return 3;
    if (!img_load()) return 4;
    return 0;
}

int run_loop(Renderer* r)
{
    SDL_Event e;
    int running = 1;

    while (running) {
        while (SDL_PollEvent(&e)) {
            handle_input(&e, &running);
        }

        SDL_SetRenderDrawColor(r->ren, 15, 20, 40, 255);
        SDL_RenderClear(r->ren);

        int img_err = run_img(r);
        if (img_err != 0) return img_err;
        // draw stuff here

        SDL_RenderPresent(r->ren);
    }

    return 0;
}

void shutdown_sdl(Renderer* r) {
    SDL_DestroyRenderer(r->ren);
    SDL_DestroyWindow(r->win);
    SDL_Quit();
}

bool img_load() {
    return true;
}

bool img_init() {
    return true;
}

bool img_kill() {
    return true;
}