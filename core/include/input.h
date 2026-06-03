#ifndef INPUT_H
#define INPUT_H

#include <SDL.h>

// WHOLE FUNCTION WILL BE REWRITEN FOR THE PI WITH THE BUTTONS THIS IS JUST FOR THE MAC

// handles Input with quitting
void handle_input(SDL_Event* e, int* running);

// Fetches mouse position
void mouse_state(int x, int y);

#endif