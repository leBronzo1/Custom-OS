#include "errors.h"
#include <stdio.h>

int error_handling(int err) {
    switch (err) {
        case 0:
            return 0;
        case 1:
            printf("Error 1: Failed to create window\n");
            return 1;
        case 2:
            printf("Error 2: Failed to create renderer\n");
            return 2;
        case 3:
            printf("Error 3: Failed to initialize image subsystem\n");
            return 3;
        case 4:
            printf("Error 4: Failed to load image\n");
            return 4;
        default:
            printf("Error %d: Unknown error\n", err);
            return -1;
    }
}