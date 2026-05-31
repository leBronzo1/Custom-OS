#include "renderer.h"
#include "input.h"
#include <stdio.h>
#include <string.h>

int init_sdl(Renderer* r) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    r->win = SDL_CreateWindow(
        "MyOS",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_SHOWN
    );

    if (!r->win) {
        SDL_Quit();
        return 2;
    }

    r->ren = SDL_CreateRenderer(
        r->win,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!r->ren) {
        SDL_DestroyWindow(r->win);
        SDL_Quit();
        return 3;
    }

    r->image1 = NULL;
    memset(r->current_cover, 0, sizeof(r->current_cover));

    return 0;
}

int run_loop(Renderer* r) {
    if (!img_init()) return 4;
    if (!img_load(r, "../../assets/icons/fun_times.bmp")) return 5;

    SDL_Event e;
    int running = 1;

    while (running) {
        while (SDL_PollEvent(&e)) {
            handle_input(&e, &running);
        }

        SDL_SetRenderDrawColor(r->ren, 15, 20, 40, 255);
        SDL_RenderClear(r->ren);

        if (r->image1) {
            SDL_RenderCopy(r->ren, r->image1, NULL, NULL);
        }

        SDL_RenderPresent(r->ren);
    }

    return 0;
}

void shutdown_sdl(Renderer* r) {
    img_kill(r);
    SDL_DestroyRenderer(r->ren);
    SDL_DestroyWindow(r->win);
    SDL_Quit();
}

bool img_load(Renderer* r, const char* path) {
    if (r->image1) SDL_DestroyTexture(r->image1);

    SDL_Surface* temp = SDL_LoadBMP(path);
    if (!temp) {
        printf("Error loading BMP: %s\n", SDL_GetError());
        return false;
    }

    r->image1 = SDL_CreateTextureFromSurface(r->ren, temp);
    SDL_FreeSurface(temp);

    if (!r->image1) {
        printf("Error creating texture: %s\n", SDL_GetError());
        return false;
    }

    strncpy(r->current_cover, path, 255);
    return true;
}

bool img_init() {
    return true;
}

bool img_kill(Renderer* r) {
    if (r->image1) {
        SDL_DestroyTexture(r->image1);
        r->image1 = NULL;
    }
    return true;
}