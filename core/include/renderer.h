#ifndef RENDERER_H
#define RENDERER_H

#include <SDL.h>
#include <stdbool.h>
#include <SDL_ttf.h>

// Struct Used throughout the rendering process and passed into each struct for uniformiality
typedef struct {
    SDL_Window* win;
    SDL_Renderer* ren;
    SDL_Texture* image1;
    TTF_Font* font;
    char current_cover[256];
} Renderer;

// Defines diffrent application states
typedef enum {
    STATE_MENU,
    STATE_GAMES,
    STATE_MUSIC,
    STATE_BOOKS
} AppState;

// Defines current application state
typedef struct {
    const char* label;
    AppState    target;
    SDL_Rect    rect;
} MenuItem;

// Holds a single music track entry from library.json
typedef struct {
    char title[256];
    char file[512];
} TrackEntry;

// Set up the base SDL window
int init_sdl(Renderer* r);

// Main loop run throughout the Program
int run_loop(Renderer* r);

// Shutdown SDL program
void shutdown_sdl(Renderer* r);

// All used to handle the loading in off an image specificall in bitmap form
bool img_load(Renderer* r, const char* path);

// Dead function I lowkey need to delete but I think I use it somewhere and I dont want to fix it
bool img_init(void);

// Delete image
bool img_kill(Renderer* r);

// Draws text at abrbritary location
bool draw_text(Renderer* r, const char* text, int x, int y, SDL_Color color);

// Handles menu logic specifically items being highlighted while selected
void menu_draw(Renderer* r, SDL_Color highlight);

// Draws game menu
void draw_games(Renderer* r);

// Draw music menu
void draw_music(Renderer* r);

// Draw book menu
void draw_books(Renderer* r);

#endif