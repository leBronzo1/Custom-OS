#ifndef INPUT_H
#define INPUT_H

#include <SDL.h>
#include <stdbool.h>

// WHOLE FUNCTION WILL BE REWRITEN FOR THE PI WITH THE BUTTONS THIS IS JUST FOR THE MAC

// Defines the mouses x and y position as well as a boolean set to false when not clikced true when clicked can also be used for Coordinates
typedef struct {
    int x;
    int y;
    bool clicked;
} Mouse;

// handles Input with quitting
void handle_input(SDL_Event* e, int* running);

// Fetches mouse position and if clicked or not
Mouse mouse_state();

#endif