#include "audio.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void audio_shutdown(void) {
    Mix_HaltMusic();
    Mix_HaltChannel(-1); // stop all sfx channels -1 halts all
    Mix_CloseAudio();
    Mix_Quit();
    printf("[audio] Shutdown complete.\n");
}

int audio_init(int frequency, int channels, int chunksize) {
    // SDL2_mixer supports MP3, OGG, WAV, FLAC, MOD out of the box.
    // MIX_INIT_MP3 | MIX_INIT_OGG covers the most common formats.
    int flags = MIX_INIT_MP3 | MIX_INIT_OGG;
    int initted = Mix_Init(flags);
 
    if ((initted & flags) != flags) {
        fprintf(stderr, "[audio] Mix_Init failed: %s\n", Mix_GetError());
        // Non-fatal: WAV still works without these flags
    }
 
    if (Mix_OpenAudio(frequency, AUDIO_S16SYS, channels, chunksize) < 0) {
        fprintf(stderr, "[audio] Mix_OpenAudio failed: %s\n", Mix_GetError());
        return -1;
    }
 
    // Allocate 16 mixing channels for simultaneous sound effects
    Mix_AllocateChannels(16);
 
    printf("[audio] Initialised  freq=%d  channels=%d  chunksize=%d\n",frequency, channels, chunksize);
    return 0;
}

SFX *audio_load_sfx(const char *path, const char *name) {
    if (!path) return NULL;
 
    Mix_Chunk *chunk = Mix_LoadWAV(path);
    if (!chunk) {
        fprintf(stderr, "[audio] Failed to load sfx '%s': %s\n", path, Mix_GetError());
        return NULL;
    }
 
    SFX *sfx = malloc(sizeof(SFX));
    if (!sfx) {
        Mix_FreeChunk(chunk);
        return NULL;
    }
 
    sfx->chunk = chunk;
    strncpy(sfx->name, name ? name : "unnamed", sizeof(sfx->name) - 1);
    sfx->name[sizeof(sfx->name) - 1] = '\0';
 
    printf("[audio] Loaded sfx: %s\n", sfx->name);
    return sfx;
}

void audio_free_sfx(SFX *sfx) {
    if (!sfx) return;
    Mix_FreeChunk(sfx->chunk);
    free(sfx);
}
 
int audio_play_sfx(SFX *sfx, int volume) {
    return audio_play_sfx_loop(sfx, volume, 0);
}
 
int audio_play_sfx_loop(SFX *sfx, int volume, int loops) {
    if (!sfx || !sfx->chunk) return -1;
 
    Mix_VolumeChunk(sfx->chunk, volume);
 
    // -1 = find first free channel
    int channel = Mix_PlayChannel(-1, sfx->chunk, loops);
    if (channel < 0) {
        fprintf(stderr, "[audio] Mix_PlayChannel failed: %s\n", Mix_GetError());
    }
    return channel;
}

MusicTrack *audio_load_music(const char *path, const char *name) {
    if (!path) return NULL;
 
    Mix_Music *music = Mix_LoadMUS(path);
    if (!music) {
        fprintf(stderr, "[audio] Failed to load music '%s': %s\n", path, Mix_GetError());
        return NULL;
    }
 
    MusicTrack *track = malloc(sizeof(MusicTrack));
    if (!track) {
        Mix_FreeMusic(music);
        return NULL;
    }
 
    track->music = music;
    strncpy(track->name, name ? name : "unnamed", sizeof(track->name) - 1);
    track->name[sizeof(track->name) - 1] = '\0';
 
    printf("[audio] Loaded music: %s\n", track->name);
    return track;
}
 
void audio_free_music(MusicTrack *track) {
    if (!track) return;
    Mix_FreeMusic(track->music);
    free(track);
}
 
void audio_play_music(MusicTrack *track, int loops, int fade_ms) {
    if (!track || !track->music) return;
 
    if (fade_ms > 0) {
        Mix_FadeInMusic(track->music, loops, fade_ms);
    } else {
        Mix_PlayMusic(track->music, loops);
    }
}
 
void audio_stop_music(int fade_ms) {
    if (fade_ms > 0) {
        Mix_FadeOutMusic(fade_ms);
    } else {
        Mix_HaltMusic();
    }
}
 
void audio_pause_music(void)  { 
    Mix_PauseMusic();
}

void audio_resume_music(void) { 
    Mix_ResumeMusic();
}

int  audio_music_playing(void) { 
    return Mix_PlayingMusic(); 
}

void audio_set_sfx_volume(int volume) {
    // -1 = apply to all channels
    Mix_Volume(-1, volume);
}
 
void audio_set_music_volume(int volume) {
    Mix_VolumeMusic(volume);
}

MusicTrack *audio_play_music_file(const char *path, const char *name, int loops, int fade_ms) {
    MusicTrack *music = audio_load_music(path, name);
    if(!music) return NULL;
    audio_play_music(music, loops, fade_ms);
    return music;
}

SFX *audio_play_sfx_file(const char *path, const char *name, int volume) {
    SFX *sfx = audio_load_sfx(path, name);
    if(!sfx) return NULL;
    audio_play_sfx(sfx, volume);
    return sfx;
}