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
    Uint32 buttons = SDL_GetMouseState(&cstate.x, &cstate.y);

    if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        cstate.clicked = true;
    } else {
        cstate.clicked = false;
    }

    return cstate;
}