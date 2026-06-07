#ifndef INPUT_H
#define INPUT_H

#include <SDL.h>

// WHOLE FUNCTION WILL BE REWRITEN FOR THE PI WITH THE BUTTONS THIS IS JUST FOR THE MAC

typedef struct {
    int x;
    int y;
} Mouse;

// handles Input with quitting
void handle_input(SDL_Event* e, int* running);

// Fetches mouse position
Mouse mouse_state();

#endif