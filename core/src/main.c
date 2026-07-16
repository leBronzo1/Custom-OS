#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
    #include <SDL.h>
    #include <SDL_ttf.h>
    #include <SDL_mixer.h>
#else
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_ttf.h>
    #include <SDL2/SDL_mixer.h>
#endif

#include "../include/renderer.h"
#include "../include/errors.h"
#include "../include/python_bridge.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    Renderer renderer = {0};

    if (init_sdl(&renderer) != 0) return 1;

    int err = run_loop(&renderer);
    error_handling(err);
    shutdown_sdl(&renderer);

    // Example of what to do when building system monitor GUI
    python_bridge_init();
    char *info = python_bridge_get_system_info();
    if (info) {
        FILE *f = fopen("../core/system_info.json", "w");
        if (f) {
            fputs(info, f);
            fclose(f);
        } else {
            perror("fopen");
        }
        free(info);
    }

    return 0;
}