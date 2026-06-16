#include "renderer.h"
#include "input.h"
#include "audio.h"
#include "cJSON.h"
#include "emulator.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static MenuItem items[] = {
    { "Games", STATE_GAMES, {300, 200, 200, 60} },
    { "Music", STATE_MUSIC, {300, 280, 200, 60} },
    { "Books", STATE_BOOKS, {300, 360, 200, 60} },
};

static const int ITEM_COUNT = 3;

#define MAX_TRACKS 256
#define MAX_ROMS   256

static TrackEntry s_tracks[MAX_TRACKS];
static int        s_track_count   = 0;
static int        s_playing_index = -1;
static MusicTrack *s_current_track = NULL;
static int        s_paused        = 0;

typedef struct {
    char title[256];
    char file[512];
    char system[64];
} RomEntry;

static RomEntry s_roms[MAX_ROMS];
static int      s_rom_count = 0;

#define TRACK_ROW_H  44
#define TRACK_LIST_X 80
#define TRACK_LIST_Y 120
#define TRACK_LIST_W 640

#define ROM_ROW_H  44
#define ROM_LIST_X 80
#define ROM_LIST_Y 120
#define ROM_LIST_W 640

#define PAUSE_BTN_W  160
#define PAUSE_BTN_H   48
#define PAUSE_BTN_X  ((800 - PAUSE_BTN_W) / 2)
#define PAUSE_BTN_Y  (600 - PAUSE_BTN_H - 24)

static void music_play_index(int index);

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

static void music_load_library(void) {
    s_track_count = 0;
    char *json = read_file("../assets/library.json");
    if (!json) { printf("[music] library.json not found\n"); return; }

    cJSON *root = cJSON_Parse(json);
    free(json);
    if (!root) return;

    cJSON *music = cJSON_GetObjectItem(root, "music");
    if (!music) { cJSON_Delete(root); return; }

    cJSON *entry;
    cJSON_ArrayForEach(entry, music) {
        if (s_track_count >= MAX_TRACKS) break;
        cJSON *title = cJSON_GetObjectItem(entry, "title");
        cJSON *file  = cJSON_GetObjectItem(entry, "file");
        if (!title || !file) continue;
        strncpy(s_tracks[s_track_count].title, title->valuestring, 255);
        strncpy(s_tracks[s_track_count].file,  file->valuestring,  511);
        s_track_count++;
    }

    cJSON_Delete(root);
    printf("[music] Loaded %d tracks\n", s_track_count);
}

static void games_load_library(void) {
    s_rom_count = 0;
    char *json = read_file("../assets/library.json");
    if (!json) { printf("[games] library.json not found\n"); return; }

    cJSON *root = cJSON_Parse(json);
    free(json);
    if (!root) return;

    cJSON *roms = cJSON_GetObjectItem(root, "roms");
    if (!roms) { cJSON_Delete(root); return; }

    cJSON *entry;
    cJSON_ArrayForEach(entry, roms) {
        if (s_rom_count >= MAX_ROMS) break;
        cJSON *title  = cJSON_GetObjectItem(entry, "title");
        cJSON *file   = cJSON_GetObjectItem(entry, "file");
        cJSON *system = cJSON_GetObjectItem(entry, "system");
        if (!title || !file) continue;
        strncpy(s_roms[s_rom_count].title,  title->valuestring,  255);
        strncpy(s_roms[s_rom_count].file,   file->valuestring,   511);
        strncpy(s_roms[s_rom_count].system, system ? system->valuestring : "Unknown", 63);
        s_rom_count++;
    }

    cJSON_Delete(root);
    printf("[games] Loaded %d ROMs\n", s_rom_count);
}

static void on_music_finished(void) {
    if (s_playing_index + 1 < s_track_count) {
        music_play_index(s_playing_index + 1);
    } else {
        if (s_current_track) { audio_free_music(s_current_track); s_current_track = NULL; }
        s_playing_index = -1;
        s_paused        = 0;
    }
}

static void music_stop_current(void) {
    audio_stop_music(0);
    if (s_current_track) { audio_free_music(s_current_track); s_current_track = NULL; }
    s_playing_index = -1;
    s_paused        = 0;
}

static void music_clear(void) {
    music_stop_current();
    s_track_count = 0;
}

static void music_play_index(int index) {
    if (index < 0 || index >= s_track_count) return;
    music_stop_current();
    Mix_HookMusicFinished(on_music_finished);
    s_current_track = audio_play_music_file(s_tracks[index].file, s_tracks[index].title, 0, 200);
    s_playing_index = s_current_track ? index : -1;
    s_paused        = 0;
}

int init_sdl(Renderer* r) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    r->win = SDL_CreateWindow("MyOS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
    if (!r->win) { SDL_Quit(); return 2; }

    r->ren = SDL_CreateRenderer(r->win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!r->ren) { SDL_DestroyWindow(r->win); SDL_Quit(); return 3; }

    if (TTF_Init() != 0) { printf("TTF_Init Error: %s\n", TTF_GetError()); return 4; }

    r->font = TTF_OpenFont("../assets/fonts/myfont.ttf", 24);
    if (!r->font) { printf("Font load error: %s\n", TTF_GetError()); return 5; }

    if (audio_init(44100, 2, 1024) != 0) return 6;
    Mix_HookMusicFinished(on_music_finished);

    r->image1 = NULL;
    memset(r->current_cover, 0, sizeof(r->current_cover));
    return 0;
}

int run_loop(Renderer* r) {
    if (!img_init()) return 4;
    if (!img_load(r, "../assets/icons/fun_times.bmp")) return 5;

    SDL_Event e;
    int running    = 1;
    AppState state = STATE_MENU;
    SDL_Color white = {255, 255, 255, 255};

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                if (state == STATE_MUSIC) music_clear();
                if (state == STATE_GAMES) s_rom_count = 0;
                state = STATE_MENU;
            }

            if (state == STATE_MUSIC && e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x;
                int my = e.button.y;

                SDL_Rect pause_btn = {PAUSE_BTN_X, PAUSE_BTN_Y, PAUSE_BTN_W, PAUSE_BTN_H};
                if (SDL_PointInRect(&(SDL_Point){mx, my}, &pause_btn) && s_playing_index >= 0) {
                    if (s_paused) { audio_resume_music(); s_paused = 0; }
                    else          { audio_pause_music();  s_paused = 1; }
                }

                for (int i = 0; i < s_track_count; i++) {
                    SDL_Rect row = {TRACK_LIST_X, TRACK_LIST_Y + i * TRACK_ROW_H, TRACK_LIST_W, TRACK_ROW_H - 4};
                    if (SDL_PointInRect(&(SDL_Point){mx, my}, &row)) { music_play_index(i); break; }
                }
            }

            if (state == STATE_GAMES && e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x;
                int my = e.button.y;

                for (int i = 0; i < s_rom_count; i++) {
                    SDL_Rect row = {ROM_LIST_X, ROM_LIST_Y + i * ROM_ROW_H, ROM_LIST_W, ROM_ROW_H - 4};
                    if (SDL_PointInRect(&(SDL_Point){mx, my}, &row)) {
                        /*
                         * Stop any playing music before launching the emulator.
                         * SDL_mixer holds the audio device; the emulator opens its
                         * own raw SDL audio device, so the two conflict if mixer is
                         * still active. music_stop_current() releases the mixer
                         * track; emulator.c then calls Mix_PauseMusic/ResumeMusic
                         * around its own audio device lifetime.
                         */
                        music_stop_current();
                        emulator_run(r->ren, s_roms[i].file);
                        /*
                         * Re-hook the finished callback after returning from the
                         * emulator, since Mix_HookMusicFinished may have been
                         * reset during the emulator session.
                         */
                        Mix_HookMusicFinished(on_music_finished);
                        break;
                    }
                }
            }

            handle_input(&e, &running);
        }

        SDL_SetRenderDrawColor(r->ren, 15, 20, 40, 255);
        SDL_RenderClear(r->ren);
        if (r->image1) SDL_RenderCopy(r->ren, r->image1, NULL, NULL);

        switch (state) {
            case STATE_MENU: {
                menu_draw(r, white);
                Mouse mouse = mouse_state();
                if (mouse.clicked) {
                    for (int i = 0; i < ITEM_COUNT; i++) {
                        if (SDL_PointInRect(&(SDL_Point){mouse.x, mouse.y}, &items[i].rect)) {
                            state = items[i].target;
                            if (state == STATE_MUSIC) music_load_library();
                            if (state == STATE_GAMES) games_load_library();
                        }
                    }
                }
                draw_text(r, "Games", 350, 215, white);
                draw_text(r, "Music", 350, 295, white);
                draw_text(r, "Books", 350, 375, white);
                break;
            }
            case STATE_GAMES: draw_games(r); break;
            case STATE_MUSIC: draw_music(r); break;
            case STATE_BOOKS: draw_books(r); break;
        }

        SDL_RenderPresent(r->ren);
    }

    return 0;
}

void shutdown_sdl(Renderer* r) {
    music_clear();
    audio_shutdown();
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
    if (!temp) { printf("Error loading BMP: %s\n", SDL_GetError()); return false; }
    r->image1 = SDL_CreateTextureFromSurface(r->ren, temp);
    SDL_FreeSurface(temp);
    if (!r->image1) { printf("Error creating texture: %s\n", SDL_GetError()); return false; }
    strncpy(r->current_cover, path, 255);
    return true;
}

bool img_init(void) { return true; }

bool img_kill(Renderer* r) {
    if (r->image1) { SDL_DestroyTexture(r->image1); r->image1 = NULL; }
    return true;
}

bool draw_text(Renderer* r, const char* text, int x, int y, SDL_Color color) {
    SDL_Surface* surf = TTF_RenderText_Blended(r->font, text, color);
    if (!surf) return false;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(r->ren, surf);
    SDL_FreeSurface(surf);
    if (!tex) return false;
    int w, h;
    SDL_QueryTexture(tex, NULL, NULL, &w, &h);
    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(r->ren, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    return true;
}

void menu_draw(Renderer* r, SDL_Color highlight) {
    (void)highlight;
    Mouse cstate = mouse_state();
    for (int i = 0; i < ITEM_COUNT; i++) {
        SDL_Rect* rect = &items[i].rect;
        SDL_bool hovered = SDL_PointInRect(&(SDL_Point){cstate.x, cstate.y}, rect);
        SDL_SetRenderDrawColor(r->ren, hovered?80:30, hovered?120:60, hovered?200:100, 255);
        SDL_RenderFillRect(r->ren, rect);
        SDL_SetRenderDrawColor(r->ren, 180, 200, 255, 255);
        SDL_RenderDrawRect(r->ren, rect);
    }
}

void draw_games(Renderer* r) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color grey  = {160, 160, 160, 255};
    SDL_Color green = {100, 220, 100, 255};

    draw_text(r, "Games", ROM_LIST_X, 70, white);

    if (s_rom_count == 0) {
        draw_text(r, "No ROMs found", ROM_LIST_X, ROM_LIST_Y, grey);
        return;
    }

    Mouse m = mouse_state();
    for (int i = 0; i < s_rom_count; i++) {
        SDL_Rect row = {ROM_LIST_X, ROM_LIST_Y + i * ROM_ROW_H, ROM_LIST_W, ROM_ROW_H - 4};
        SDL_bool hovered = SDL_PointInRect(&(SDL_Point){m.x, m.y}, &row);

        if (hovered) SDL_SetRenderDrawColor(r->ren, 30, 80, 50, 255);
        else         SDL_SetRenderDrawColor(r->ren, 20, 30, 60, 200);
        SDL_RenderFillRect(r->ren, &row);

        SDL_SetRenderDrawColor(r->ren, 60, 140, 80, 255);
        SDL_RenderDrawRect(r->ren, &row);

        char label[320];
        snprintf(label, sizeof(label), "%s  [%s]", s_roms[i].title, s_roms[i].system);
        draw_text(r, label, ROM_LIST_X + 12, row.y + 10, hovered ? green : white);
    }
}

void draw_books(Renderer* r) {
    SDL_Color white = {255, 255, 255, 255};
    draw_text(r, "Books Page", 300, 100, white);
    draw_text(r, "Your library goes here", 250, 180, white);
}

void draw_music(Renderer* r) {
    SDL_Color white  = {255, 255, 255, 255};
    SDL_Color grey   = {160, 160, 160, 255};
    SDL_Color yellow = {255, 220,  60, 255};

    draw_text(r, "Music", TRACK_LIST_X, 70, white);

    if (s_track_count == 0) {
        draw_text(r, "No tracks found", TRACK_LIST_X, TRACK_LIST_Y, grey);
    } else {
        Mouse m = mouse_state();
        for (int i = 0; i < s_track_count; i++) {
            SDL_Rect row = {TRACK_LIST_X, TRACK_LIST_Y + i * TRACK_ROW_H, TRACK_LIST_W, TRACK_ROW_H - 4};
            SDL_bool hovered = SDL_PointInRect(&(SDL_Point){m.x, m.y}, &row);

            if      (i == s_playing_index) SDL_SetRenderDrawColor(r->ren, 40, 80, 160, 255);
            else if (hovered)              SDL_SetRenderDrawColor(r->ren, 30, 50,  90, 255);
            else                           SDL_SetRenderDrawColor(r->ren, 20, 30,  60, 200);
            SDL_RenderFillRect(r->ren, &row);

            SDL_SetRenderDrawColor(r->ren, 60, 80, 140, 255);
            SDL_RenderDrawRect(r->ren, &row);

            char label[300];
            if (i == s_playing_index) {
                snprintf(label, sizeof(label), "> %s", s_tracks[i].title);
                draw_text(r, label, TRACK_LIST_X + 12, row.y + 10, yellow);
            } else {
                draw_text(r, s_tracks[i].title, TRACK_LIST_X + 12, row.y + 10, white);
            }
        }
    }

    SDL_Rect pause_btn  = {PAUSE_BTN_X, PAUSE_BTN_Y, PAUSE_BTN_W, PAUSE_BTN_H};
    SDL_bool btn_active = (s_playing_index >= 0);

    SDL_SetRenderDrawColor(r->ren, btn_active?50:30, btn_active?100:50, btn_active?180:80, 255);
    SDL_RenderFillRect(r->ren, &pause_btn);
    SDL_SetRenderDrawColor(r->ren, 180, 200, 255, 255);
    SDL_RenderDrawRect(r->ren, &pause_btn);

    const char *btn_label = !btn_active ? "Pause" : (s_paused ? "Resume" : "Pause");
    SDL_Color btn_color   = btn_active ? white : grey;

    SDL_Surface *surf = TTF_RenderText_Blended(r->font, btn_label, btn_color);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(r->ren, surf);
        int tw, th;
        SDL_QueryTexture(tex, NULL, NULL, &tw, &th);
        SDL_Rect label_rect = {PAUSE_BTN_X + (PAUSE_BTN_W - tw) / 2, PAUSE_BTN_Y + (PAUSE_BTN_H - th) / 2, tw, th};
        SDL_RenderCopy(r->ren, tex, NULL, &label_rect);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
}