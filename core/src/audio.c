#include "audio.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void audio_shutdown(void){
    Mix_HaltMusic();
    Mix_HaltChannel(-1); // stop all sfx channels -1 halts all
    Mix_CloseAudio();
    Mix_Quit();
    printf("[audio] Shutdown complete.\n");
}