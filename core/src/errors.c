#include "errors.h"
#include <stdio.h>

int error_handling(int err) {
    switch (err) {
        case 0:
            return 0;
        case 1:
            printf("Error 1: SDL_Init failed\n");
            return 1;
        case 2:
            printf("Error 2: Failed to create window\n");
            return 2;
        case 3:
            printf("Error 3: Failed to create renderer\n");
            return 3;
        case 4:
            printf("Error 4: TTF_Init failed / img_init failed\n");
            return 4;
        case 5:
            printf("Error 5: Failed to load font / Failed to load image\n");
            return 5;
        case 6:
            printf("Error 6: audio_init failed\n");
            return 6;
        default:
            printf("Error %d: Unknown error\n", err);
            return -1;
    }
}