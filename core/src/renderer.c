#include "renderer.h"
#include "input.h"
#include <stdio.h>
#include <string.h>

static MenuItem items[] = {
    { "Games", STATE_GAMES, {300, 200, 200, 60} },
    { "Music", STATE_MUSIC, {300, 280, 200, 60} },
    { "Books", STATE_BOOKS, {300, 360, 200, 60} },
};

static const int ITEM_COUNT = 3;

int init_sdl(Renderer* r) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    r->win = SDL_CreateWindow(
        "MyOS",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_SHOWN
    );
    if (!r->win) { SDL_Quit(); return 2; }

    r->ren = SDL_CreateRenderer(
        r->win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!r->ren) { SDL_DestroyWindow(r->win); SDL_Quit(); return 3; }

    // TTF init here, but no drawing
    if (TTF_Init() != 0) {
        printf("TTF_Init Error: %s\n", TTF_GetError());
        return 4;
    }

    r->font = TTF_OpenFont("assets/fonts/myfont.ttf", 24);
    if (!r->font) {
        printf("Font load error: %s\n", TTF_GetError());
        return 5;
    }

    r->image1 = NULL;
    memset(r->current_cover, 0, sizeof(r->current_cover));
    return 0;
}

int run_loop(Renderer* r) {
    if (!img_init()) return 4;
    if (!img_load(r, "../assets/icons/fun_times.bmp")) return 5;

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
    if (r->font) TTF_CloseFont(r->font);
    TTF_Quit();
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

bool draw_text(Renderer* r, const char* text, int x, int y, SDL_Color color) {
    SDL_Surface* surf = TTF_RenderText_Blended(r->font, text, color);
    if(!surf) return false;

    SDL_Texture* tex = SDL_CreateTextureFromSurface(r->ren, surf);
    SDL_FreeSurface(surf);
    if (!tex) return;

    int w, h;
    SDL_QueryTexture(tex, NULL, NULL, &w, &h);
    SDL_Rect label_rect = { x, y, w, h };

    SDL_RenderCopy(r->ren, tex, NULL, &label_rect);
    SDL_DestroyTexture(tex);
}