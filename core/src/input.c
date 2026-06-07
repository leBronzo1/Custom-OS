#include "input.h"

void handle_input(SDL_Event* e, int* running)
{
    if (e->type == SDL_QUIT)
        *running = 0;

    if (e->type == SDL_KEYDOWN) {
        switch (e->key.keysym.sym) {
            case SDLK_ESCAPE: *running = 0; break;
            case SDLK_q: *running = 0; break;
        }
    }
}

Mouse mouse_state(){
    Mouse cstate;
    SDL_GetMouseState(&cstate.x, &cstate.y);
    return cstate;
}